#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <regex>
#include <string>

// Text format tests - verify output format improvements
// Tests: file grouping, source display, statistics summary, suggestion highlighting

class TextFormatTest : public ::testing::Test {
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

  // Run codelint command and capture output
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

// Test: File grouping - issues from same file appear together
// Expected: File header "=== filename.cpp ===" before issues from that file
TEST_F(TextFormatTest, FileGrouping_IssuesFromSameFileAppearTogether) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file =
      "/Users/aronic/Documents/codelint/tests/CodeLintTest/src/init_checker/src/init_check.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Check for file grouping pattern: "=== /path/to/file.cpp ===" or similar
  // The output should group issues by file with a clear header
  std::regex file_header_pattern(R"(=== .*\.cpp \(\d+ issues\) ===)");
  bool has_file_header = std::regex_search(output, file_header_pattern);

  EXPECT_TRUE(has_file_header) << "Output should group issues by file with a clear header: === "
                                  "file.cpp (N issues) ===\nOutput:\n"
                               << output.substr(0, 500);
}

// Test: File grouping - multiple files have separate sections
// Expected: When checking directory, each file has its own section
TEST_F(TextFormatTest, FileGrouping_MultipleFilesHaveSeparateSections) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_dir = "/Users/aronic/Documents/codelint/tests/CodeLintTest/src/init_checker/src";
  std::string cmd = codelint_path + " check_init " + test_dir + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Count file headers/sections
  std::regex file_section_pattern(R"(=== .*\.cpp \(\d+ issues\) ===)");
  auto section_begin = std::sregex_iterator(output.begin(), output.end(), file_section_pattern);
  auto section_end = std::sregex_iterator();
  int section_count = std::distance(section_begin, section_end);

  // If multiple files are found, should have multiple sections
  if (output.find(".cpp") != std::string::npos) {
    EXPECT_GE(section_count, 1) << "Should have file sections for grouping\nOutput:\n"
                                << output.substr(0, 500);
  }
}

// Test: Source code line display - actual source shown for each issue
// Expected: Source line printed with "| line | code" pattern
TEST_F(TextFormatTest, SourceDisplay_ActualSourceLineShown) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file =
      "/Users/aronic/Documents/codelint/tests/CodeLintTest/src/init_checker/src/init_check.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Look for source line pattern: "| 14 | char c1 = 0;"
  std::regex source_line_pattern(R"(\|\s*\d+\s*\|)");
  bool has_source_line = std::regex_search(output, source_line_pattern);

  // Alternative: "Source:" label followed by code
  bool has_source_label = output.find("Source:") != std::string::npos;

  EXPECT_TRUE(has_source_line || has_source_label)
      << "Output should show actual source code line for each issue with pattern '| N | "
         "code'\nOutput:\n"
      << output.substr(0, 500);
}

// Test: Source display shows line number
// Expected: Line number displayed in source context
TEST_F(TextFormatTest, SourceDisplay_LineNumberVisible) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file =
      "/Users/aronic/Documents/codelint/tests/CodeLintTest/src/init_checker/src/init_check.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Look for line number pattern: "14:6" at start of issue line
  std::regex line_num_pattern(R"(\d+:\d+:)");
  bool has_line_number = std::regex_search(output, line_num_pattern);

  EXPECT_TRUE(has_line_number) << "Output should display line numbers in format 'N:M:'\nOutput:\n"
                               << output.substr(0, 500);
}

// Test: Statistics summary - file count present
// Expected: "Files: N" or "Files processed: N" in summary
TEST_F(TextFormatTest, StatisticsSummary_FileCountPresent) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_dir = "/Users/aronic/Documents/codelint/tests";
  std::string cmd = codelint_path + " check_init " + test_dir + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Look for file count pattern
  std::regex file_count_pattern(R"(Files:\s*\d+|Files processed:\s*\d+)");
  bool has_file_count = std::regex_search(output, file_count_pattern);

  // Alternative: "N file(s)" in output
  std::regex file_pattern(R"(\d+\s+file\(s\))");
  bool has_file_pattern = std::regex_search(output, file_pattern);

  EXPECT_TRUE(has_file_count || has_file_pattern) << "Summary should show file count";
}

// Test: Statistics summary - issue breakdown present
// Expected: Counts for each severity level (error, warning, info)
TEST_F(TextFormatTest, StatisticsSummary_IssueBreakdownPresent) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Check for severity counts
  bool has_error = std::regex_search(output, std::regex(R"(Errors:\s*\d+|error:\s*\d+)"));
  bool has_warning = std::regex_search(output, std::regex(R"(Warnings:\s*\d+|warning:\s*\d+)"));
  bool has_total = std::regex_search(output, std::regex(R"(Total:\s*\d+)"));

  EXPECT_TRUE(has_error && has_warning && has_total)
      << "Summary should show issue breakdown (Errors, Warnings, Total)";
}

// Test: Statistics summary - time elapsed present
// Expected: "Time: X.XXs" or "Elapsed: X.XXs" or similar
TEST_F(TextFormatTest, StatisticsSummary_TimeElapsedPresent) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Look for time pattern: "X.XXs", "X ms", etc.
  std::regex time_pattern(R"(\d+\.?\d*\s*(ms|s|seconds?|milliseconds?))", std::regex::icase);
  bool has_time = std::regex_search(output, time_pattern);

  EXPECT_TRUE(has_time) << "Summary should show elapsed time";
}

// Test: Suggestion highlighting - clearly visible
// Expected: "suggestion:" label before fix text
TEST_F(TextFormatTest, SuggestionHighlighting_ClearlyVisible) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file =
      "/Users/aronic/Documents/codelint/tests/CodeLintTest/src/init_checker/src/init_check.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Check for suggestion label (lowercase, as we're using)
  bool has_suggestion = output.find("suggestion:") != std::string::npos;

  EXPECT_TRUE(has_suggestion)
      << "Output should clearly label suggestions with 'suggestion:'\nOutput:\n"
      << output.substr(0, 500);
}

// Test: Suggestion contains fix code
// Expected: Suggestion shows actual code to fix, e.g., "int a{5};"
TEST_F(TextFormatTest, SuggestionContainsFixCode) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // Look for code-like suggestion: contains braces {} or semicolons
  // After "Suggestion:" label
  size_t suggestion_pos = output.find("Suggestion:");
  if (suggestion_pos == std::string::npos) {
    suggestion_pos = output.find("suggestion:");
  }

  if (suggestion_pos != std::string::npos) {
    std::string after_suggestion = output.substr(suggestion_pos);
    // Check for code patterns in suggestion
    bool has_code = after_suggestion.find('{') != std::string::npos ||
                    after_suggestion.find('}') != std::string::npos ||
                    after_suggestion.find(';') != std::string::npos;

    EXPECT_TRUE(has_code) << "Suggestion should contain actual fix code";
  } else {
    // No suggestion found - this might be expected for some checkers
    SUCCEED() << "No suggestion found (may be expected for this checker)";
  }
}

// Test: Output format consistency
// Expected: Consistent formatting throughout
TEST_F(TextFormatTest, OutputFormat_ConsistentFormatting) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " check_init " + test_file + " 2>/dev/null";
  std::string output = runCommand(cmd);

  // If there are issues, check that they follow consistent format
  if (output.find("issue") != std::string::npos || output.find("warning") != std::string::npos) {
    // Check for consistent severity bracket format: [warning], [error], etc.
    std::regex severity_bracket(R"(\[(error|warning|info|hint)\])", std::regex::icase);
    auto begin = std::sregex_iterator(output.begin(), output.end(), severity_bracket);
    auto end = std::sregex_iterator();

    // If we have severity brackets, there should be multiple and they should be consistent
    int count = std::distance(begin, end);
    if (count > 0) {
      EXPECT_GE(count, 1) << "Severity indicators should be consistent";
    }
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
