#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

// Edge case tests for command utilities
class EdgeCasesTest : public ::testing::Test {
protected:
  std::string codelint_path;
  std::string long_line_fixture;

  void SetUp() override {
    codelint_path = "/Users/aronic/Documents/codelint/build/codelint";
    long_line_fixture = "/Users/aronic/Documents/codelint/tests/fixtures/edge_cases/long_line.cpp";
  }

  // Helper to run command and capture output
  std::string runCommand(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
      return "";

    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      output += buffer;
    }
    pclose(pipe);
    return output;
  }
};

// Test with empty directory
TEST_F(EdgeCasesTest, EmptyDirectory) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string empty_dir = "/tmp/empty_cpp_dir";
  std::filesystem::create_directory(empty_dir);

  std::string cmd = codelint_path + " find_global " + empty_dir;
  int result = system(cmd.c_str());

  std::filesystem::remove(empty_dir);
  EXPECT_EQ(result / 256, 1); // system() returns exit_code * 256
}

// Test with non-existent path
TEST_F(EdgeCasesTest, NonExistentPath) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " find_global /nonexistent/path/12345";
  int result = system(cmd.c_str());
  EXPECT_EQ(result / 256, 1); // system() returns exit_code * 256
}

// Test with nested directories
TEST_F(EdgeCasesTest, NestedDirectories) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string nested = "/tmp/nested_cpp_test/level1/level2";
  std::filesystem::create_directories(nested);
  std::ofstream(nested + "/deep.cpp") << "int x;";

  std::string cmd = codelint_path + " find_global /tmp/nested_cpp_test";
  int result = system(cmd.c_str());

  std::filesystem::remove_all("/tmp/nested_cpp_test");
  EXPECT_TRUE(result == 0 || result == 1);
}

// Test truncation in text output
TEST_F(EdgeCasesTest, LongLineTruncationText) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " check_init " + long_line_fixture + " 2>&1";
  std::string output = runCommand(cmd);

  // Verify truncation indicator "..." present
  EXPECT_TRUE(output.find("...") != std::string::npos) << "Truncation indicator not found";

  // Find the source line in output and verify length <= 123 (120 + "...")
  size_t line_pos = output.find("| 14 |");
  if (line_pos != std::string::npos) {
    std::string source_line =
        output.substr(line_pos + 7); // Skip "| 14 |" (7 chars including space)
    size_t end_pos = source_line.find('\n');
    if (end_pos != std::string::npos) {
      source_line = source_line.substr(0, end_pos);
    }
    // Source line should be exactly 120 chars + "..." = 123 chars
    EXPECT_EQ(source_line.length(), 123u)
        << "Truncated line length incorrect: " << source_line.length();
  }
}

// Test no truncation in SARIF output
TEST_F(EdgeCasesTest, LongLineNoTruncationSarif) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " --output-sarif check_init " + long_line_fixture + " 2>&1";
  std::string output = runCommand(cmd);

  // SARIF should have full replacement text (no truncation)
  // The original line is ~945 chars, SARIF should preserve it
  EXPECT_TRUE(output.length() > 1000) << "SARIF output appears truncated";

  // Check that the long string is present in the replacement
  EXPECT_TRUE(output.find("Lorem ipsum dolor sit amet") != std::string::npos);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
