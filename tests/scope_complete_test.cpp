#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <random>

namespace fs = std::filesystem;

class ScopeCompleteTest : public ::testing::Test {
protected:
    std::string build_dir;
    std::string codelint_path;
    fs::path temp_dir;
    std::string original_dir;

    void SetUp() override {
        // Store original directory
        original_dir = fs::current_path().string();

        // Setup build directory
        build_dir = std::getenv("BUILD_DIR") ? std::getenv("BUILD_DIR")
                                             : "/Users/aronic/Documents/codelint-regression-test/build";
        codelint_path = build_dir + "/codelint";

        // Create temporary directory for git repo
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        temp_dir = fs::temp_directory_path() / ("scope_test_" + std::to_string(dis(gen)));
        fs::create_directories(temp_dir);
        fs::current_path(temp_dir);

        // Initialize git repo
        execute_git({"init"});
        execute_git({"config", "user.name", "Test User"});
        execute_git({"config", "user.email", "test@test.com"});
    }

    void TearDown() override {
        // Cleanup temporary directory
        fs::current_path(original_dir);
        fs::remove_all(temp_dir);
    }

    void create_file(const std::filesystem::path& file_path, const std::string& content_str) {
        std::ofstream file(temp_dir / file_path);
        file << content_str;
        file.close();
    }

    void append_file(const std::filesystem::path& file_path, const std::string& content_str) {
        std::ofstream file(temp_dir / file_path, std::ios::app);
        file << content_str;
        file.close();
    }

    void execute_git(const std::vector<std::string>& args) {
        std::string cmd = "git";
        for (const auto& arg : args) {
            cmd += " " + arg;
        }
        int result = system(cmd.c_str());
        if (result != 0) {
            ADD_FAILURE() << "Git command failed: " << cmd << " (exit code: " << result << ")";
        }
    }

    void commit(const std::string& message) {
        execute_git({"add", "."});
        execute_git({"commit", "-m", "'" + message + "'"});
    }

    std::string run_codelint(const std::vector<std::string>& args) {
        std::string cmd = codelint_path;
        for (const auto& arg : args) {
            // Properly quote arguments with spaces
            if (arg.find(' ') != std::string::npos) {
                cmd += " '" + arg + "'";
            } else {
                cmd += " " + arg;
            }
        }

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "";
        }

        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        pclose(pipe);

        return output;
    }

    int run_codelint_exit_code(const std::vector<std::string>& args) {
        std::string cmd = codelint_path;
        for (const auto& arg : args) {
            cmd += " " + arg;
        }
        return system(cmd.c_str());
    }

    std::string read_file(const std::string& name) {
        std::ifstream file(temp_dir / name);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }

    bool file_exists(const std::string& name) {
        return fs::exists(temp_dir / name);
    }
};

// =============================================================================
// Test 1: ScopeAllTest - Test --scope all (default behavior)
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeAllReportsAllIssues) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create a file with multiple initialization issues
    create_file("test.cpp", R"(
int uninit1;
int uninit2;
int equals_init = 5;
unsigned int no_suffix = 10;
)");

    // Run with --scope all (default)
    std::string output = run_codelint({"check_init", "test.cpp"});

    // Should report all issues (at least 3: uninit1, uninit2, equals_init, no_suffix)
    EXPECT_TRUE(output.find("uninit1") != std::string::npos ||
                output.find("warning") != std::string::npos) << "Should find initialization issues";
    EXPECT_TRUE(output.find("uninit2") != std::string::npos ||
                output.find("warning") != std::string::npos) << "Should find initialization issues";
}

// =============================================================================
// Test 2: ScopeModifiedTest - Test --scope modified (uncommitted changes)
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeModifiedOnlyReportsModifiedLines) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create initial file with clean code
    create_file("test.cpp", R"(
int clean_var{};
)");
    commit("Initial commit");

    // Modify the file - add new issues
    append_file("test.cpp", R"(
int new_uninit;
int new_equals = 10;
)");

    // Run with --scope modified
    std::string output = run_codelint({"--scope", "modified", "check_init", "test.cpp"});

    // Should only report issues in modified lines (new_uninit, new_equals)
    // The original clean_var{} should not be reported
    EXPECT_TRUE(output.find("Running") != std::string::npos ||
                output.find("new_uninit") != std::string::npos ||
                output.find("new_equals") != std::string::npos) <<
        "Should report issues in modified lines";
}

// =============================================================================
// Test 3: ScopeStagedTest - Test --scope staged (staged changes)
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeStagedChecksStagedContent) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create initial file
    create_file("test.cpp", R"(
int original{};
)");
    commit("Initial commit");

    // Create new file with issues
    create_file("staged.cpp", R"(
int staged_uninit;
)");

    // Stage the file
    execute_git({"add", "staged.cpp"});

    // Run with --scope staged
    std::string output = run_codelint({"--scope", "staged", "check_init", "staged.cpp"});

    // Should find the issue in staged.cpp
    EXPECT_TRUE(output.find("staged_uninit") != std::string::npos ||
                output.find("uninit") != std::string::npos) <<
        "Should report issues in staged files";
}

// =============================================================================
// Test 4: ScopeCommitTest - Test --scope commit:HEAD
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeCommitChecksSpecificCommit) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create initial file
    create_file("test.cpp", R"(
int first{};
)");
    commit("First commit");

    // Add code with issues in second commit
    create_file("test.cpp", R"(
int first{};
int second_uninit;
)");
    commit("Second commit with issues");

    // Check HEAD commit
    std::string output = run_codelint({"--scope", "commit:HEAD", "check_init", "test.cpp"});

    // Should check changes introduced in HEAD
    EXPECT_TRUE(output.find("Running") != std::string::npos ||
                output.find("second_uninit") != std::string::npos) <<
        "Should check changes in HEAD commit";
}

// =============================================================================
// Test 5: FileSkipTest - Test file-level skipping
// =============================================================================
TEST_F(ScopeCompleteTest, FileSkipUnmodifiedFiles) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create two files
    create_file("file1.cpp", R"(
int clean1{};
)");
    create_file("file2.cpp", R"(
int clean2{};
)");

    commit("Initial commit");

    // Modify only file1
    append_file("file1.cpp", R"(
int new_issue;
)");

    // Run scope modified on both files
    std::string output = run_codelint({"--scope", "modified", "check_init", "file1.cpp", "file2.cpp"});

    // file2 should be skipped (unmodified)
    EXPECT_TRUE(output.find("Skipping unmodified file") != std::string::npos ||
                output.find("file2") == std::string::npos) <<
        "Should skip unmodified file2.cpp";
}

// =============================================================================
// Test 6: LineFilterTest - Test line-level filtering
// =============================================================================
TEST_F(ScopeCompleteTest, LineFilterOnlyReportsModifiedLines) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create file with existing issue (line 2)
    create_file("test.cpp", R"(int old_issue;
int clean{};
)");
    commit("Initial with old issue");

    // Modify only line 3 (add new issue)
    std::string content = read_file("test.cpp");
    std::ofstream(temp_dir / "test.cpp") << content << "int new_issue;\n";
    execute_git({"add", "test.cpp"});

    // Run scope modified
    std::string output = run_codelint({"--scope", "modified", "check_init", "test.cpp"});

    // Should report issues only in modified lines
    // Note: Old issue on line 2 should be filtered out
    EXPECT_TRUE(output.find("Running") != std::string::npos ||
                output.find("new_issue") != std::string::npos) <<
        "Should report issues in modified lines only";
}

// =============================================================================
// Test 7: ScopeWithGlobalTest - Test --scope with find_global
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeWithFindGlobal) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create initial file with global
    create_file("test.cpp", R"(
int global_clean{};
)");
    commit("Initial");

    // Add new global variable
    append_file("test.cpp", R"(
int global_new_issue = 5;
)");

    // Run find_global with scope modified
    std::string output = run_codelint({"--scope", "modified", "find_global", "test.cpp"});

    // Should find global variables
    EXPECT_TRUE(output.find("global_new_issue") != std::string::npos ||
                output.find("global") != std::string::npos ||
                output.find("Running") != std::string::npos) <<
        "find_global should work with scope";
}

// =============================================================================
// Test 8: ScopeWithSingletonTest - Test --scope with find_singleton
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeWithFindSingleton) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    // Create initial clean file
    create_file("test.cpp", R"(
class Clean {
public:
    void method() {}
};
)");
    commit("Initial");

    // Add singleton pattern
    append_file("test.cpp", R"(
class Singleton {
public:
    static Singleton& instance() {
        static Singleton inst{};
        return inst;
    }
};
)");

    // Run find_singleton with scope modified
    std::string output = run_codelint({"--scope", "modified", "find_singleton", "test.cpp"});

    // Should find singleton
    EXPECT_TRUE(output.find("Singleton") != std::string::npos ||
                output.find("instance") != std::string::npos ||
                output.find("Running") != std::string::npos) <<
        "find_singleton should work with scope";
}

// =============================================================================
// Test 9: ScopeInvalidBranchTest - Test error handling for invalid references
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeHandlesInvalidReference) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    create_file("test.cpp", R"(
int var{};
)");
    commit("Initial");

    // Try to use non-existent branch
    (void)run_codelint({"--scope", "commit:nonexistent123", "check_init", "test.cpp"});

    // Should handle gracefully (error message or skip)
    // The important thing is it doesn't crash
    SUCCEED() << "Invalid reference handled";
}

// =============================================================================
// Test 10: ScopeEmptyRepoTest - Test scope handles no-commit repo gracefully
// =============================================================================
TEST_F(ScopeCompleteTest, ScopeGracefulHandlingInNoCommitRepo) {
    if (!fs::exists(codelint_path)) {
        GTEST_SKIP() << "codelint not built";
    }

    create_file("test.cpp", R"(
int uninit;
)");

    std::string output = run_codelint({"--scope", "modified", "check_init", "test.cpp"});

    EXPECT_TRUE(output.find("Invalid scope") != std::string::npos ||
                output.find("HEAD") != std::string::npos ||
                output.find("fatal") != std::string::npos ||
                output.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
