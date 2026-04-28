// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17
void test_for_statements() {
  int x = 1;
  bool b = true;

  for (; x;) {
// CHECK-MESSAGES: :[@LINE]:10: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  for (; 1;) {
// CHECK-MESSAGES: :[@LINE]:10: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  for (; nullptr;) {
// CHECK-MESSAGES: :[@LINE]:10: warning: condition must be bool type, but got 'nullptr_t'  [codelint-strict-bool-condition]
  }

  for (; b;) {
  }
  for (; true;) {
  }
  for (; x == 1;) {
  }
}
