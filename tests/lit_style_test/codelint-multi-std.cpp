// RUN: %check_codelint -std=c++11,c++14,c++17 %s codelint-lint-code %t -- -std=c++17

void test_unsigned_suffix() {
  unsigned u1 = 1;
  unsigned u2 = 100U;
  unsigned long ul = 100;
  uint64_t u64 = 42;
  int x = u1 + ul;
}

// Test with multiple C++ standards - only u1, ul, u64 should warn
// CHECK-MESSAGES: :[[@LINE-5]]:14: warning: missing 'U' suffix for unsigned type [codelint-lint-code]
// CHECK-MESSAGES: :[[@LINE-4]]:14: warning: missing 'UL' suffix for unsigned long type [codelint-lint-code]
// CHECK-MESSAGES: :[[@LINE-3]]:14: warning: missing 'UL' suffix for uint64_t type [codelint-lint-code]
