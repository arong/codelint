// RUN: %check_codelint %s codelint-init %t -- -std=c++17
void testb() {
  bool flag = false;
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void testb() {
// CHECK-FIXES:   bool flag{false};
// CHECK-FIXES: }
