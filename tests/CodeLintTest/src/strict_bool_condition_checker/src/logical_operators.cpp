void test_logical_operators() {
  int x = 1;
  const char* s = "hello";
  bool b = true;
  double d = 3.14;

  // Logical NOT on non-bool types
  if (!x) {
  }
  if (!s) {
  }
  if (!d) {
  }

  // Logical AND with non-bool operands
  if (x && x) {
  }
  if (s && s) {
  }
  if (x && b) {
  }
  if (b && x) {
  }
  if (s && b) {
  }
  if (b && s) {
  }
  if (x && s) {
  }

  // Logical OR with non-bool operands
  if (x || b) {
  }
  if (b || x) {
  }
  if (s || b) {
  }
  if (b || s) {
  }
  if (x || s) {
  }

  // Nested logical expressions
  if (!x && b) {
  }
  if (!s || b) {
  }
  if (x && b && true) {
  }
  if (b || x || false) {
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
