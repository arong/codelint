// RUN: %codelint %s codelint-strict-bool-condition %t
void test_for_statements() {
  int x = 1;
  bool b = true;

  for (; x;) {
// CHECK-MESSAGES: :6:10: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  for (; 1;) {
// CHECK-MESSAGES: :9:10: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  for (; nullptr;) {
// CHECK-MESSAGES: :12:10: warning: condition must be bool type, but got 'nullptr_t'  [codelint-strict-bool-condition]
  }

  for (; b;) {
  }
  for (; true;) {
  }
  for (; x == 1;) {
  }
}
