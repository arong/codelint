// RUN: %check_codelint %s codelint-init %t -- -std=c++17

int x;
void test() {
  int local_uninit;
  int* ptr = nullptr;
}

// CHECK-MESSAGES: :[[@LINE-4]]:1: error: uninitialized trivial type 'x' [codelint-init]
// CHECK-MESSAGES: :[[@LINE-3]]:3: warning: function 'test' is not used [clang-diagnostic-unused-function]
// CHECK-MESSAGES: :[[@LINE-2]]:3: error: uninitialized trivial type 'local_uninit' [codelint-init]
