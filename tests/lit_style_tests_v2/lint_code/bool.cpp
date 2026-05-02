// RUN: %check_codelint %s codelint-lint-code %t
void testb() {
  bool flag = false;
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: variable should use '{}' syntax for initialization
  // [codelint-lint-code]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void testb() {
// CHECK-FIXES:   bool flag{false};
// CHECK-FIXES: }
