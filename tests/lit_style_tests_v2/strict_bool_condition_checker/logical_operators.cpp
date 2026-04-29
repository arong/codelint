// RUN: %codelint %s codelint-strict-bool-condition %t
void test_logical_operators() {
  int x = 1;
  const char* s = "hello";
  bool b = true;
  double d = 3.14;

  // Logical NOT on non-bool types
  if (!x) {
// CHECK-MESSAGES: :9:8: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (!s) {
// CHECK-MESSAGES: :12:8: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (!d) {
// CHECK-MESSAGES: :15:8: warning: condition must be bool type, but got 'double'  [codelint-strict-bool-condition]
  }

  // Logical AND with non-bool operands
  if (x && x) {
// CHECK-MESSAGES: :20:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s && s) {
// CHECK-MESSAGES: :23:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && b) {
// CHECK-MESSAGES: :26:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b && x) {
// CHECK-MESSAGES: :29:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s && b) {
// CHECK-MESSAGES: :32:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (b && s) {
// CHECK-MESSAGES: :35:12: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && s) {
// CHECK-MESSAGES: :38:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }

  // Logical OR with non-bool operands
  if (x || b) {
// CHECK-MESSAGES: :43:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b || x) {
// CHECK-MESSAGES: :46:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s || b) {
// CHECK-MESSAGES: :49:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (b || s) {
// CHECK-MESSAGES: :52:12: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x || s) {
// CHECK-MESSAGES: :55:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }

  // Nested logical expressions
  if (!x && b) {
// CHECK-MESSAGES: :60:8: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (!s || b) {
// CHECK-MESSAGES: :63:8: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && b && true) {
// CHECK-MESSAGES: :66:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b || x || false) {
// CHECK-MESSAGES: :69:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }

  // Valid cases - all bool operands
  if (!b) {
  }
  if (b && b) {
  }
  if (b || b) {
  }
  if (!b && b) {
  }
  if (b || !b) {
  }
}
