// Test for C-style array initialization
// Focus: static arrays, multi-dimensional arrays (macro_array.cpp covers macro-defined)

#include <cassert>

// 1. SINGLE DIMENSION C-ARRAY
int arr1d[10]{};

// 2. MULTI-DIMENSIONAL ARRAYS
int arr2d[3][4]{};
int arr3d[2][3][4]{};

// 3. LOCAL C-ARRAY
void test_local_array() {
  int local_arr[5]{};
  double local_arr2d[2][3]{};
}

// 4. C-ARRAY IN MAIN
int main() {
  int x1{}, x2{}, x3{};
  assert(x1 == 0);
  return 0;
}