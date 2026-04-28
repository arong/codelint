// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17
void test_do_while_statements() {
  int x = 1;
  bool b = true;

  do {
  } while (x);
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  do {
  } while (1);
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'int'  [codelint-strict-bool-condition]
  do {
  } while (nullptr);
// CHECK-MESSAGES: :[@LINE]:12: warning: condition must be bool type, but got 'nullptr_t'  [codelint-strict-bool-condition]

  do {
  } while (b);
  do {
  } while (true);
  do {
  } while (x == 1);
}
