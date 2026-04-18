void test_ternary_operator() {
  int x = 1;
  bool b = true;

  int a1 = x ? 1 : 0;
  int a2 = 1 ? 1 : 0;
  int a3 = nullptr ? 1 : 0;

  int a4 = b ? 1 : 0;
  int a5 = true ? 1 : 0;
  int a6 = (x == 1) ? 1 : 0;
}
