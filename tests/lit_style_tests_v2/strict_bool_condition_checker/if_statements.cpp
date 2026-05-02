// RUN: %check_codelint %s codelint-strict-bool-condition %t
void test_if_statements() {
  int x = 1;
  const char* s = "hello";
  bool b = true;

  if (x) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  if (s) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'const char *'
    // [codelint-strict-bool-condition]
  }
  if (nullptr) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'nullptr_t'
    // [codelint-strict-bool-condition]
  }
  if (1) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  if (0) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  if (3.14) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'double'
    // [codelint-strict-bool-condition]
  }

  if (b) {
  }
  if (true) {
  }
  if (false) {
  }
  if (x == 1) {
  }
  if (s != nullptr) {
  }
  if (x > 0 && b) {
  }
  if (!s) { // shall also be reported
    // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: condition must be bool type, but got 'const char *'
    // [codelint-strict-bool-condition]
  }
}
