// RUN: %check_codelint %s codelint-init %t
// Test file for InitCheck clang-tidy plugin - macro and array functionality

// Macro test cases (should NOT trigger warnings)
#define MACRO_VAR int macro_x
MACRO_VAR; // This should NOT trigger a warning

#define DECLARE_VAR(type, name) type name
DECLARE_VAR(int, macro_y); // This should NOT trigger a warning

#define CREATE_ARRAY(type, name, size) type name[size];
CREATE_ARRAY(int, macro_arr, 5); // This should NOT trigger a warning

int regular_var; // This SHOULD trigger a warning
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: variable is not initialized  [codelint-init]
int regular_array[5]; // This SHOULD trigger a C-style array warning
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: C-style array is not initialized  [codelint-init]
int initialized_array[5] = {};           // This SHOULD trigger a warning (suggest brace init)
int brace_initialized[5]{};              // This should NOT trigger a warning
int assigned_array[5] = {1, 2, 3, 4, 5}; // This SHOULD trigger a warning (suggest brace init)
float float_array[10];                   // This SHOULD trigger a C-style array warning
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: C-style array is not initialized  [codelint-init]

void function_test() {
  int local_var; // This SHOULD trigger a warning
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: variable is not initialized  [codelint-init]
  char char_arr[20]; // This SHOULD trigger a C-style array warning
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: C-style array is not initialized  [codelint-init]

// Macro tests inside function (should NOT trigger warnings)
#define LOCAL_MACRO int local_macro_var
  LOCAL_MACRO; // This should NOT trigger a warning

#define DECL_LOCAL_ARR(type, name, size) type name[size];
  DECL_LOCAL_ARR(char, local_macro_arr, 10); // This should NOT trigger a warning
}

// Define different types of macros to test various scenarios
#define SIMPLE_VAR int a
SIMPLE_VAR; // This should NOT trigger a warning

#define FUNC_MOCK(name) int mock_##name
FUNC_MOCK(test_func); // This should NOT trigger a warning

// Regular variable without initialization (should trigger warning)
double uninit_double; // This SHOULD trigger a warning
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: variable is not initialized  [codelint-init]

// Regular C-style arrays (should trigger warnings)
bool bool_flags[25]; // This SHOULD trigger a C-style array warning
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: C-style array is not initialized  [codelint-init]
char buffer[1024]; // This SHOULD trigger a C-style array warning
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: C-style array is not initialized  [codelint-init]
