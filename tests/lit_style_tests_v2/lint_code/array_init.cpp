// RUN: %check_codelint %s codelint-lint-code %t

void test_array_equals_init() {
  char buf[] = "hello";
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: variable should use '{}' syntax for initialization
  // [codelint-lint-code]
  int a[3] = {1, 2, 3};
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: initializer should use '{}' syntax instead of '= {}'
  // [codelint-lint-code]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void test_array_equals_init() {
// CHECK-FIXES:   char buf[]{"hello"};
// CHECK-FIXES:   int a[3]{1, 2, 3};
// CHECK-FIXES: }
