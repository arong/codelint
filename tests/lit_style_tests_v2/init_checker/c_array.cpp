// RUN: %check_codelint %s codelint-init %t
// Test for C-style array initialization
// Focus: static arrays, multi-dimensional arrays (macro_array.cpp covers macro-defined)

#include <cassert>

// 1. SINGLE DIMENSION C-ARRAY
int arr1d[10];
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: C-style array is not initialized  [codelint-init]

// 2. MULTI-DIMENSIONAL ARRAYS
int arr2d[3][4];
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: C-style array is not initialized  [codelint-init]
int arr3d[2][3][4];
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: C-style array is not initialized  [codelint-init]

// 3. LOCAL C-ARRAY
void test_local_array() {
  int local_arr[5];
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: C-style array is not initialized  [codelint-init]
  double local_arr2d[2][3];
  // CHECK-MESSAGES: :[[@LINE-1]]:10: error: C-style array is not initialized  [codelint-init]
}

// 4. C-ARRAY IN MAIN
int main() {
  int x1, x2, x3;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:11: error: variable is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-3]]:15: error: variable is not initialized  [codelint-init]
  assert(x1 == 0);

  int arr[3] = {1, 2, 3};
  int arr2[3] = {1, 2, 3};
  return 0;
}
