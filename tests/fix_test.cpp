#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class FixTest : public ::testing::Test {
protected:
    std::string build_dir;
    std::string codelint_path;
    fs::path temp_dir;

    void SetUp() override {
        build_dir = std::getenv("BUILD_DIR") ? std::getenv("BUILD_DIR")
                                             : "/Users/aronic/Documents/codelint/build";
        codelint_path = build_dir + "/codelint";
        temp_dir = fs::temp_directory_path() / ("fix_test_" + std::to_string(std::rand()));
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }

    bool codelintExists() {
        return fs::exists(codelint_path);
    }

    void create_test_file(const std::string& filename, const std::string& content) {
        fs::path file_path = temp_dir / filename;
        std::ofstream out(file_path);
        out << content;
        out.close();
    }

    std::string read_file(const std::string& filename = "test.cpp") {
        fs::path file_path = temp_dir / filename;
        std::ifstream in(file_path);
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    std::string run_codelint_check_init_fix(const std::string& file_path) {
        std::string cmd = codelint_path + " check_init " + file_path + " --fix --inplace 2>&1";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    (void)result;
    return result;
    }

    bool files_equal(const std::string& content1, const std::string& content2) {
        std::string normalized1 = content1;
        std::string normalized2 = content2;
        normalized1.erase(std::remove(normalized1.begin(), normalized1.end(), '\r'), normalized1.end());
        normalized2.erase(std::remove(normalized2.begin(), normalized2.end(), '\r'), normalized2.end());
        return normalized1 == normalized2;
    }

    std::string get_test_file_path(const std::string& filename = "test.cpp") {
        return (temp_dir / filename).string();
    }
};

TEST_F(FixTest, FixBasicTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main() {
    int a;
    int b = 5;
    unsigned c = 42;
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_NE(original_content, fixed_content) << "File should be modified by --fix";

    EXPECT_TRUE(fixed_content.find("int a{};") != std::string::npos ||
                fixed_content.find("int a{ }") != std::string::npos)
        << "Uninitialized variable 'a' should be fixed to 'int a{};'";

    EXPECT_TRUE(fixed_content.find("int b{5};") != std::string::npos ||
                fixed_content.find("int b{ 5 }") != std::string::npos)
        << "Variable 'b' should use brace initialization";

    EXPECT_TRUE(fixed_content.find("c{42U}") != std::string::npos ||
                fixed_content.find("c{ 42U }") != std::string::npos)
        << "Unsigned variable 'c' should have 'U' suffix";
}

TEST_F(FixTest, FixSafetyTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(// This is a comment
int main() {
    int properly_initialized{5};  // Already correct
    int another{10};              // Already correct
    
    // Important comment here
    int needs_fix;
    
    return 0;  // Return statement
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.find("// This is a comment") != std::string::npos)
        << "Comments should be preserved";

    EXPECT_TRUE(fixed_content.find("int properly_initialized{5};") != std::string::npos)
        << "Already correct lines should not be modified";

    EXPECT_TRUE(fixed_content.find("int another{10};") != std::string::npos)
        << "Already correct lines should not be modified";

    EXPECT_TRUE(fixed_content.find("// Important comment here") != std::string::npos)
        << "Comments should be preserved";

    EXPECT_TRUE(fixed_content.find("// Return statement") != std::string::npos)
        << "End-of-line comments should be preserved";

    EXPECT_TRUE(fixed_content.find("int needs_fix{}") != std::string::npos)
        << "The uninitialized variable should be fixed";
}

TEST_F(FixTest, FixIdempotencyTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main() {
    int x;
    int y = 10;
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string first_fix_content = read_file();

    run_codelint_check_init_fix(test_file);
    std::string second_fix_content = read_file();

    EXPECT_TRUE(files_equal(first_fix_content, second_fix_content))
        << "--fix should be idempotent: running twice should produce same result";
}

TEST_F(FixTest, FixPartialFailureTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main() {
    int a;
    int b;
    int c = 5;
    unsigned d = 100;
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.length() > 0) 
        << "Should process without crashing";

    EXPECT_TRUE(fixed_content.find("a{}") != std::string::npos ||
                fixed_content.find("b{}") != std::string::npos ||
                fixed_content.find("c{5}") != std::string::npos ||
                fixed_content.find("d{100U}") != std::string::npos)
        << "At least some fixes should be applied";
}

TEST_F(FixTest, FixWithScopeTest) {
    GTEST_SKIP() << "Scope test requires git repository setup - skipped for now";
}

TEST_F(FixTest, FixWithJsonOutputTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main() {
    int x;
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        FAIL() << "Failed to run codelint";
    }

    char buffer[4096];
    std::string json_output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        json_output += buffer;
    }
    pclose(pipe);
    (void)json_output;

    EXPECT_TRUE(json_output.find("\"issues\"") != std::string::npos)
        << "JSON output should contain 'issues' field";
}

TEST_F(FixTest, FixConstCheckerTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main() {
    int x{42};
    int y{x};
    return y;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.length() > 0)
        << "File should be processed successfully";

    EXPECT_TRUE(fixed_content.find("int y{") != std::string::npos)
        << "Variables should use brace initialization";
}

TEST_F(FixTest, FixComplexFileTest) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(#include <iostream>

// Complex file with multiple issues
int main() {
    int a;           // Problem 1: uninitialized
    int b = 5;       // Problem 2: should use brace init
    unsigned c = 10; // Problem 3: missing U suffix
    
    const int d = 20; // OK
    int e{30};        // OK
    
    int x = a + b + c + d + e;
    
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.find("a{}") != std::string::npos)
        << "Uninitialized variable 'a' should be fixed";

    EXPECT_TRUE(fixed_content.find("b{5}") != std::string::npos)
        << "Variable 'b' should use brace initialization";

    EXPECT_TRUE(fixed_content.find("c{10U}") != std::string::npos)
        << "Unsigned 'c' should have U suffix";

    EXPECT_TRUE(fixed_content.find("const int d = 20;") != std::string::npos ||
                fixed_content.find("const int d{20};") != std::string::npos)
        << "Already const variable should remain unchanged";

    EXPECT_TRUE(fixed_content.find("int e{30};") != std::string::npos)
        << "Already correct brace init should remain unchanged";

    EXPECT_TRUE(fixed_content.find("// Complex file") != std::string::npos)
        << "Comments should be preserved";
}

TEST_F(FixTest, FixMultipleVariablesInFunction) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(void foo() {
    int a, b, c;
    int d = 1, e = 2;
}

int main() {
    foo();
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.find("a{}") != std::string::npos ||
                fixed_content.find("b{}") != std::string::npos ||
                fixed_content.find("c{}") != std::string::npos)
        << "Multiple uninitialized variables in one declaration should be fixed";
}

TEST_F(FixTest, FixPreservesCodeStyle) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string original_content = R"(int main()
{
    // Tab-indented code
	int needs_fix;
    // Space-indented code
    int also_needs_fix;
    return 0;
}
)";
    std::string test_file = get_test_file_path();
    create_test_file("test.cpp", original_content);

    run_codelint_check_init_fix(test_file);
    std::string fixed_content = read_file();

    EXPECT_TRUE(fixed_content.find("needs_fix{}") != std::string::npos)
        << "Tab-indented variable should be fixed";

    EXPECT_TRUE(fixed_content.find("also_needs_fix{}") != std::string::npos)
        << "Space-indented variable should be fixed";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
