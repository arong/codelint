// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17
void test_valid_bool_conditions() {
  bool flag = true;
  bool condition = false;

  if (flag) {
  }
  if (condition) {
  }
  if (true) {
  }
  if (false) {
  }

  while (flag) {
  }
  while (condition) {
  }

  for (; flag;) {
  }

  do {
  } while (condition);

  int x = flag ? 1 : 0;
  int y = true ? 1 : 0;

  bool result = flag && condition;
  bool result2 = flag || condition;
  bool result3 = !flag;

  if (result) {
  }
  if (result2) {
  }
  if (result3) {
  }
}
