# Edge Case Test Fixtures

This directory contains test fixtures for edge case handling in codelint.

## Files

### clean.cpp
- **Purpose**: Test empty results handling
- **Content**: Valid C++ code with NO issues for codelint to detect
- **Expected behavior**: codelint should report 0 issues
- **Use case**: Verify that empty result handling works correctly

### utf8.cpp
- **Purpose**: Test UTF-8 character handling
- **Content**: Valid C++ with Chinese characters, emoji, Korean, and Japanese
- **Expected behavior**: codelint should handle UTF-8 without crashing
- **Use case**: Verify Unicode/UTF-8 handling in source code analysis

### long_line.cpp
- **Purpose**: Test long line buffer handling
- **Content**: Contains a line with more than 500 characters
- **Expected behavior**: codelint should handle long lines without truncation or crash
- **Use case**: Verify line buffer can handle very long lines

### multi_issue.cpp
- **Purpose**: Test multiple issues on same line
- **Content**: Multiple codelint-detectable issues on a single line
- **Expected behavior**: codelint should report all issues (use_equals_init, unsigned_without_suffix, etc.)
- **Use case**: Verify multi-issue reporting per line

## Usage

These fixtures are used for Tasks 12-14 (edge case implementations):
- Task 12: Empty results handling
- Task 13: UTF-8 character handling
- Task 14: Long line handling

Run codelint against these files:
```bash
./build/codelint check_init tests/fixtures/edge_cases/clean.cpp
./build/codelint check_init tests/fixtures/edge_cases/utf8.cpp
./build/codelint check_init tests/fixtures/edge_cases/long_line.cpp
./build/codelint check_init tests/fixtures/edge_cases/multi_issue.cpp
```

## Notes

- All files contain valid C++ that compiles successfully
- These are NOT in the protected test directory (`tests/CodeLintTest/`)
- Safe to modify for edge case testing purposes
