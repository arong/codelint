// RUN: %check_codelint %s codelint-init %t -- -std=c++17
// Test for compilation error detection
// When header file is missing, the check should NOT produce false suggestions
// This tests the hasErrorOccurred() early-exit logic

#include "nonexistent_header.h" // This header does not exist

void test_with_missing_header() {
  int uninitialized; // Should NOT be reported because compilation failed
  int another = 5;   // Should NOT be reported because compilation failed
}

int main() {
  int x; // Should NOT be reported because compilation failed
  return 0;
}
