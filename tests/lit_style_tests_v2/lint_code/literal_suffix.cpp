// RUN: %check_codelint %s codelint-lint-code %t

void test_lowercase_suffix() {
  int d = 7ul;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: variable should use '{}' syntax for initialization
  // {{.*}}[codelint-lint-code] CHECK-MESSAGES: :[[@LINE-2]]:11: warning: integer literal suffix
  // should be uppercase {{.*}}[codelint-lint-code]
  unsigned x = 0x1fu;
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: variable should use '{}' syntax for initialization
  // {{.*}}[codelint-lint-code] CHECK-MESSAGES: :[[@LINE-2]]:16: warning: integer literal suffix
  // should be uppercase {{.*}}[codelint-lint-code]
  long long y = 1ull;
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable should use '{}' syntax for initialization
  // {{.*}}[codelint-lint-code] CHECK-MESSAGES: :[[@LINE-2]]:17: warning: integer literal suffix
  // should be uppercase {{.*}}[codelint-lint-code]
  unsigned ok = 1U;
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: variable should use '{}' syntax for initialization
  // {{.*}}[codelint-lint-code]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void test_lowercase_suffix() {
// CHECK-FIXES:   int d{7UL};
// CHECK-FIXES:   unsigned x{0x1fU};
// CHECK-FIXES:   long long y{1ULL};
// CHECK-FIXES:   unsigned ok{1U};
// CHECK-FIXES: }
