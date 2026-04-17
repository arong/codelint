void test_for_statements() {
  int x = 1;
  bool b = true;

  for (; x;) {
  }
  for (; 1;) {
  }
  for (; nullptr;) {
  }

  for (; b;) {
  }
  for (; true;) {
  }
  for (; x == 1;) {
  }
}
