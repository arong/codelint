// RUN: %codelint %s codelint-init %t
// Test for multiple declarators on one line
// P0-1: All variables in multi-declarator statements should be checked

void test_multiple_declarators() {
  int a, b, c; // All three should trigger warnings
               // CHECK-MESSAGES: :6:7: error: variable is not initialized  [codelint-init]
               // CHECK-MESSAGES: :6:10: error: variable is not initialized  [codelint-init]
               // CHECK-MESSAGES: :6:13: error: variable is not initialized  [codelint-init]
  double x = 1.0, y, z = 3.0; // Only y should trigger warning
  // CHECK-MESSAGES: :10:19: error: variable is not initialized  [codelint-init]
  char *p1, *p2, *p3; // All three should trigger warnings
                      // CHECK-MESSAGES: :12:9: error: variable is not initialized  [codelint-init]
                      // CHECK-MESSAGES: :12:14: error: variable is not initialized  [codelint-init]
                      // CHECK-MESSAGES: :12:19: error: variable is not initialized  [codelint-init]

  int arr1[5], arr2[10]; // Both should trigger C-style array warnings
  // CHECK-MESSAGES: :17:7: error: C-style array is not initialized  [codelint-init]
  // CHECK-MESSAGES: :17:16: error: C-style array is not initialized  [codelint-init]
  float f1{}, f2, f3{}; // Only f2 should trigger warning
  // CHECK-MESSAGES: :20:15: error: variable is not initialized  [codelint-init]
}

void test_mixed_initialization() {
  int initialized = 10, uninitialized, also_initialized = 20; // Only uninitialized should warn
  // CHECK-MESSAGES: :25:25: error: variable is not initialized  [codelint-init]
  unsigned u1 = 1U, u2, u3 = 3U; // u2 should warn, and suggest U suffix
  // CHECK-MESSAGES: :27:21: error: variable is not initialized  [codelint-init]
}

void test_single_declarators() {
  int single;        // Should trigger warning
                     // CHECK-MESSAGES: :32:7: error: variable is not initialized  [codelint-init]
  int initialized{}; // Should NOT trigger warning
}

class TestClass {
public:
  void method() {
    int m1, m2, m3; // All three should trigger warnings
                    // CHECK-MESSAGES: :40:9: error: variable is not initialized  [codelint-init]
                    // CHECK-MESSAGES: :40:13: error: variable is not initialized  [codelint-init]
                    // CHECK-MESSAGES: :40:17: error: variable is not initialized  [codelint-init]
    bool b1 = true, b2; // Both should trigger warnings
    // CHECK-MESSAGES: :44:21: error: variable is not initialized  [codelint-init]
  }
};

int global1, global2, global3; // All three should trigger warnings
// CHECK-MESSAGES: :49:5: error: variable is not initialized  [codelint-init]
// CHECK-MESSAGES: :49:14: error: variable is not initialized  [codelint-init]
// CHECK-MESSAGES: :49:23: error: variable is not initialized  [codelint-init]

static int s1, s2{}; // Only s1 should trigger warning
// CHECK-MESSAGES: :54:12: error: variable is not initialized  [codelint-init]

// === Expected Fixed Output ===
// CHECK-FIXES: void test_multiple_declarators() {
// CHECK-FIXES:   int a{}, b{}, c{};          // All three should trigger warnings
// CHECK-FIXES:   double x{1.0}, y{}, z{3.0}; // Only y should trigger warning
// CHECK-FIXES:   char *p1{}, *p2{}, *p3{};   // All three should trigger warnings
// CHECK-FIXES:   int arr1[5]{}, arr2[10]{}; // Both should trigger C-style array warnings
// CHECK-FIXES:   float f1{}, f2{}, f3{};    // Only f2 should trigger warning
// CHECK-FIXES: }
// CHECK-FIXES: void test_mixed_initialization() {
// CHECK-FIXES:   int initialized{10}, uninitialized{}, also_initialized{20}; // Only uninitialized
// should warn CHECK-FIXES:   unsigned u1{1U}, u2{}, u3{3U}; // u2 should warn, and suggest U suffix
// CHECK-FIXES: }
// CHECK-FIXES: void test_single_declarators() {
// CHECK-FIXES:   int single{};      // Should trigger warning
// CHECK-FIXES:   int initialized{}; // Should NOT trigger warning
