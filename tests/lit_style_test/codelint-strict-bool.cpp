// RUN: %check_codelint %s codelint-strict-bool-condition %t -- -std=c++17

int getValue();

void test_if(int status) {
  if (status) {}
  while (status) {}
  for (; status; ) {}
  do {} while (status);
  bool b = status ? 1 : 0;
}

// CHECK-MESSAGES: :[[@LINE-6]]:3: error: condition expression type 'int' is not bool [codelint-strict-bool-condition]
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: condition expression type 'int' is not bool [codelint-strict-bool-condition]
// CHECK-MESSAGES: :[[@LINE-4]]:3: error: condition expression type 'int' is not bool [codelint-strict-bool-condition]
// CHECK-MESSAGES: :[[@LINE-3]]:3: error: condition expression type 'int' is not bool [codelint-strict-bool-condition]
// CHECK-MESSAGES: :[[@LINE-1]]:16: error: condition expression type 'int' is not bool [codelint-strict-bool-condition]
