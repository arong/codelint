// RUN: %check_codelint %s codelint-init %t
// Test for multiple declarators on one line
// P0-1: All variables in multi-declarator statements should be checked

void test_multiple_declarators() {
  int a, b,
      c; // All three should trigger warnings
         // CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]
         // CHECK-MESSAGES: :[[@LINE-2]]:10: error: variable is not initialized  [codelint-init]
         // CHECK-MESSAGES: :[[@LINE-3]]:13: error: variable is not initialized  [codelint-init]
  double x = 1.0, y, z = 3.0; // Only y should trigger warning
  // CHECK-MESSAGES: :[[@LINE-1]]:19: error: variable is not initialized  [codelint-init]
  char *p1, *p2, *p3; // All three should trigger warnings
  // CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:14: error: variable is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-3]]:19: error: variable is not initialized  [codelint-init]

  int arr1[5], arr2[10]; // Both should trigger C-style array warnings
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: C-style array is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:16: error: C-style array is not initialized  [codelint-init]
  float f1{}, f2, f3{}; // Only f2 should trigger warning
  // CHECK-MESSAGES: :[[@LINE-1]]:15: error: variable is not initialized  [codelint-init]
}

void test_mixed_initialization() {
  int initialized = 10, uninitialized, also_initialized = 20; // Only uninitialized should warn
  // CHECK-MESSAGES: :[[@LINE-1]]:25: error: variable is not initialized  [codelint-init]
  unsigned u1 = 1U, u2, u3 = 3U; // u2 should warn, and suggest U suffix
  // CHECK-MESSAGES: :[[@LINE-1]]:21: error: variable is not initialized  [codelint-init]
}

void test_single_declarators() {
  int single; // Should trigger warning
              // CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]
  int initialized{}; // Should NOT trigger warning
}

class TestClass {
public:
  void method() {
    int m1, m2, m3; // All three should trigger warnings
    // CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
    // CHECK-MESSAGES: :[[@LINE-2]]:13: error: variable is not initialized  [codelint-init]
    // CHECK-MESSAGES: :[[@LINE-3]]:17: error: variable is not initialized  [codelint-init]
    bool b1 = true, b2; // Both should trigger warnings
    // CHECK-MESSAGES: :[[@LINE-1]]:21: error: variable is not initialized  [codelint-init]
  }
};

int global1, global2, global3; // All three should trigger warnings
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: variable is not initialized  [codelint-init]
// CHECK-MESSAGES: :[[@LINE-2]]:14: error: variable is not initialized  [codelint-init]
// CHECK-MESSAGES: :[[@LINE-3]]:23: error: variable is not initialized  [codelint-init]

static int s1, s2{}; // Only s1 should trigger warning
// CHECK-MESSAGES: :[[@LINE-1]]:12: error: variable is not initialized  [codelint-init]
