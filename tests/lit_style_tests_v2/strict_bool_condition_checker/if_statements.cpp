// RUN: %codelint %s codelint-strict-bool-condition %t
void test_if_statements() {
  int x = 1;
  const char* s = "hello";
  bool b = true;

  if (x) {
// CHECK-MESSAGES: :7:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s) {
// CHECK-MESSAGES: :10:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (nullptr) {
// CHECK-MESSAGES: :13:7: warning: condition must be bool type, but got 'nullptr_t'  [codelint-strict-bool-condition]
  }
  if (1) {
// CHECK-MESSAGES: :16:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (0) {
// CHECK-MESSAGES: :19:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (3.14) {
// CHECK-MESSAGES: :22:7: warning: condition must be bool type, but got 'double'  [codelint-strict-bool-condition]
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
// CHECK-MESSAGES: :38:8: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
}
