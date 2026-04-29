// RUN: %check_codelint %s codelint-lint-code %t -- -std=c++17

void test() {
  int x = 5;
  unsigned u = 100;
  uint64_t big = 42;
}

// CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use {} initialization instead of = [codelint-lint-code]
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: missing 'U' suffix for unsigned type [codelint-lint-code]
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: missing 'UL' suffix for uint64_t type [codelint-lint-code]
