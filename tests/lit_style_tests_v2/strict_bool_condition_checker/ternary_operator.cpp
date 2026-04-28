// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17
void test_ternary_operator() {
  int x = 1;
  bool b = true;

  int a1 = x ? 1 : 0;
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  int a2 = 1 ? 1 : 0;
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  int a3 = nullptr ? 1 : 0;
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'nullptr_t'  [codelint-strict-bool-condition]

  int a4 = b ? 1 : 0;
  int a5 = true ? 1 : 0;
  int a6 = (x == 1) ? 1 : 0;
}
