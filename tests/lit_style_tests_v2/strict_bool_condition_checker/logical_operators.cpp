// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17
void test_logical_operators() {
  int x = 1;
  const char* s = "hello";
  bool b = true;
  double d = 3.14;

  // Logical NOT on non-bool types
  if (!x) {
// CHECK-MESSAGES: :[@LINE]:8: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (!s) {
// CHECK-MESSAGES: :[@LINE]:8: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (!d) {
// CHECK-MESSAGES: :[@LINE]:8: warning: condition must be bool type, but got 'double'  [codelint-strict-bool-condition]
  }

  // Logical AND with non-bool operands
  if (x && x) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s && s) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && b) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b && x) {
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s && b) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (b && s) {
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && s) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }

  // Logical OR with non-bool operands
  if (x || b) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b || x) {
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (s || b) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (b || s) {
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x || s) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }

  // Nested logical expressions
  if (!x && b) {
// CHECK-MESSAGES: :[@LINE]:8: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (!s || b) {
// CHECK-MESSAGES: :[@LINE]:8: warning: condition must be bool type, but got 'const char *'  [codelint-strict-bool-condition]
  }
  if (x && b && true) {
// CHECK-MESSAGES: :[@LINE]:7: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  }
  if (b || x || false) {
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
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
