// RUN: %codelint %s codelint-init %t
// Test for local variable initialization in functions
// Focus: function scope, equals vs brace style

#include <string>

// 1. UNINITIALIZED LOCAL VARIABLES
void test_uninit_local() {
  int local1;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: variable is not initialized  [codelint-init]
  double local3;
  // CHECK-MESSAGES: :[@LINE-1]:10: error: variable is not initialized  [codelint-init]
  char local4;
  // CHECK-MESSAGES: :[@LINE-1]:8: error: variable is not initialized  [codelint-init]
  bool local5;
  // CHECK-MESSAGES: :[@LINE-1]:8: error: variable is not initialized  [codelint-init]
}

// 2. EQUALS STYLE (should suggest brace)
void test_equals_local() {
  int local6 = 10;
  double local7 = 3.14;
  int a = 1;
}

// 3. BRACE STYLE (OK - no warning)
void test_brace_local() {
  int local_ok{20};
  double local_ok2{2.5};
}

// 4. NON-BUILTIN TYPES (should NOT warn)
void test_nonbuiltin_local() {
  std::string local_str("hello");
}

// 5. COMPLEX SCENARIO
struct ComplexStruct {
  int x;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  double y;
  // CHECK-MESSAGES: :[@LINE-1]:10: error: field is not initialized  [codelint-init]
};
void test_complex_local() {
  ComplexStruct cs;
  // CHECK-MESSAGES: :[@LINE-1]:17: error: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <string>
// CHECK-FIXES: void test_uninit_local() {
// CHECK-FIXES:   int local1{};
// CHECK-FIXES:   double local3{};
// CHECK-FIXES:   char local4{};
// CHECK-FIXES:   bool local5{};
// CHECK-FIXES: }
// CHECK-FIXES: void test_equals_local() {
// CHECK-FIXES:   int local6{10};
// CHECK-FIXES:   double local7{3.14};
// CHECK-FIXES:   int a{1};
// CHECK-FIXES: }
