// RUN: %check_codelint %s codelint-strict-bool-condition %t
void test_while_statements() {
  int x = 1;
  const char* s = "hello";
  bool b = true;

  while (x) {
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  while (s) {
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: condition must be bool type, but got 'const char *'
    // [codelint-strict-bool-condition]
  }
  while (nullptr) {
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: condition must be bool type, but got 'nullptr_t'
    // [codelint-strict-bool-condition]
  }
  while (1) {
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }

  while (b) {
  }
  while (true) {
  }
  while (x == 1) {
  }
}
