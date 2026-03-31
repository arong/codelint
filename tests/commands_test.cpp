#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <cstdlib>

// Integration tests - these test the actual command behavior
// We'll test by examining the output of the compiled codelint binary

class CommandsTest : public ::testing::Test {
protected:
    std::string build_dir;
    std::string codelint_path;

    void SetUp() override {
        build_dir = std::getenv("BUILD_DIR") ? std::getenv("BUILD_DIR") : "/Users/aronic/Documents/codelint/build";
        codelint_path = build_dir + "/codelint";
    }

    bool codelintExists() {
        return std::filesystem::exists(codelint_path);
    }
};

// Test collect_cpp_files via find_global (which uses it internally)
TEST_F(CommandsTest, CollectsCppFilesFromDirectory) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string test_dir = "/Users/aronic/Documents/codelint/tests";
    std::string cmd = codelint_path + " find_global " + test_dir;
    int result = system(cmd.c_str());
    EXPECT_TRUE(result == 0 || result == 1);
}

// Test print_statistics output
TEST_F(CommandsTest, CheckInitShowsStatistics) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
    std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);

    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    EXPECT_TRUE(output.find("Statistics") != std::string::npos ||
                output.find("Files processed") != std::string::npos);
}

// Test that find_global command exists and works
TEST_F(CommandsTest, FindGlobalCommandWorks) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string test_file = "/Users/aronic/Documents/codelint/tests/test_find_global.cpp";
    std::string cmd = codelint_path + " find_global " + test_file;
    int result = system(cmd.c_str());
    EXPECT_TRUE(result == 0 || result == 1);
}

// Test that find_singleton command exists and works
TEST_F(CommandsTest, FindSingletonCommandWorks) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string test_file = "/Users/aronic/Documents/codelint/tests/test_find_singleton.cpp";
    std::string cmd = codelint_path + " find_singleton " + test_file;
    int result = system(cmd.c_str());
    EXPECT_TRUE(result == 0 || result == 1);
}

// Test check_init command works
TEST_F(CommandsTest, CheckInitCommandWorks) {
    if (!codelintExists()) {
        GTEST_SKIP() << "codelint not built";
    }

    std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
    std::string cmd = codelint_path + " check_init " + test_file;
    int result = system(cmd.c_str());
    EXPECT_TRUE(result == 0 || result == 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
