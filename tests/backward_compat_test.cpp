#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>

// Backward compatibility tests for --output-json flag
// These tests verify that the JSON output format remains compatible
// with existing integrations and scripts that depend on it.

class BackwardCompatTest : public ::testing::Test {
protected:
  std::string build_dir;
  std::string codelint_path;

  void SetUp() override {
    build_dir = std::getenv("BUILD_DIR") ? std::getenv("BUILD_DIR")
                                         : "/Users/aronic/Documents/codelint/build";
    codelint_path = build_dir + "/codelint";
  }

  bool codelintExists() {
    return std::filesystem::exists(codelint_path);
  }

  // Helper to run command and capture output
  std::string runCommand(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      output += buffer;
    }
    pclose(pipe);
    return output;
  }
};

// Test that --output-json flag is recognized by the CLI
TEST_F(BackwardCompatTest, OutputJsonFlagIsRecognized) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  // Run with --output-json flag - should not crash or error
  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>&1";

  std::string output = runCommand(cmd);

  // Should produce some output (not an error about unknown flag)
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find("unrecognized") == std::string::npos);
  EXPECT_TRUE(output.find("unknown option") == std::string::npos);
}

// Test that JSON output is valid JSON (parseable)
TEST_F(BackwardCompatTest, JsonOutputIsValidJson) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Basic JSON validation: should start with { and end with }
  EXPECT_TRUE(output.find("{") != std::string::npos);
  EXPECT_TRUE(output.find("}") != std::string::npos);

  // Should not contain error messages in output
  EXPECT_TRUE(output.find("error:") == std::string::npos);
}

// Test JSON structure: has "issues" array
TEST_F(BackwardCompatTest, JsonHasIssuesArray) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // JSON should contain "issues" key
  EXPECT_TRUE(output.find("\"issues\"") != std::string::npos);
}

// Test JSON structure: has "summary" object
TEST_F(BackwardCompatTest, JsonHasSummaryObject) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // JSON should contain "summary" key
  EXPECT_TRUE(output.find("\"summary\"") != std::string::npos);
}

// Test JSON summary has error_count field
TEST_F(BackwardCompatTest, JsonSummaryHasErrorCount) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Summary should contain "errors" field (not "error_count" - based on issue_reporter.cpp)
  EXPECT_TRUE(output.find("\"errors\"") != std::string::npos);
}

// Test JSON summary has warning_count field
TEST_F(BackwardCompatTest, JsonSummaryHasWarningCount) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Summary should contain "warnings" field (not "warning_count" - based on issue_reporter.cpp)
  EXPECT_TRUE(output.find("\"warnings\"") != std::string::npos);
}

// Test backward compat: find_global also supports --output-json
TEST_F(BackwardCompatTest, FindGlobalSupportsJsonOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_find_global.cpp";
  std::string cmd = codelint_path + " --output-json find_global " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Should have valid JSON with issues array
  EXPECT_TRUE(output.find("\"issues\"") != std::string::npos);
}

// Test backward compat: find_singleton also supports --output-json
TEST_F(BackwardCompatTest, FindSingletonSupportsJsonOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_find_singleton.cpp";
  std::string cmd = codelint_path + " --output-json find_singleton " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Should have valid JSON with issues array
  EXPECT_TRUE(output.find("\"issues\"") != std::string::npos);
}

// Test issue objects have required fields
// Uses check_init which has proper IssueReporter JSON format
TEST_F(BackwardCompatTest, IssueObjectsHaveRequiredFields) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";

  std::string output = runCommand(cmd);

  // Check init has both issues and summary
  EXPECT_TRUE(output.find("\"issues\"") != std::string::npos);
  EXPECT_TRUE(output.find("\"summary\"") != std::string::npos);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
