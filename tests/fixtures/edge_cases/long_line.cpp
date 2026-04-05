// long_line.cpp - Edge case fixture for long line handling
// Contains a line with more than 500 characters for testing line buffer handling
// Expected: codelint should handle long lines without truncation or crash

#include <iostream>
#include <string>
#include <vector>

// This line contains more than 500 characters to test the line buffer handling in codelint
// The line below contains more than 500 characters including the line content
// This is important to test because some codebases have very long lines especially in
// template-heavy code or auto-generated code We need to ensure that codelint can handle these long
// lines without truncating or crashing The line should be detected as a single line and not split
// incorrectly

// clang-format off
int long_line_with_issue = 42; // This is a very long line to test the line buffer handling in codelint The line below contains more than 500 characters including the line content This is important to test because some codebases have very long lines especially in template-heavy code or auto-generated code We need to ensure that codelint can handle these long lines without truncating or crashing The line should be detected as a single line and not split incorrectly This ensures the test for truncation works as expected when checking for lines that exceed normal formatting limits
// clang-format on
const std::string very_long_string_constant =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut "
    "labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco "
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat "
    "cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. This "
    "line is intentionally very long to test the line buffer handling in codelint. The line below "
    "contains more than 500 characters including the line content. This is important to test "
    "because some codebases have very long lines especially in template-heavy code or "
    "auto-generated code. We need to ensure that codelint can handle these long lines without "
    "truncating or crashing. The line should be detected as a single line and not split "
    "incorrectly.";

int main() {
  std::cout << "Long line test" << std::endl;
  std::cout << very_long_string_constant.length() << std::endl;
  return 0;
}
