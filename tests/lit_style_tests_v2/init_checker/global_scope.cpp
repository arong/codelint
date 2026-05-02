// RUN: %check_codelint %s codelint-init %t
// Test for global/static variable initialization
// Focus: global scope, static, extern

// 1. GLOBAL VARIABLES
int global_var1;
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: variable is not initialized  [codelint-init]
static int static_global1;
// CHECK-MESSAGES: :[[@LINE-1]]:12: error: variable is not initialized  [codelint-init]

// 2. EXTERN DECLARATION (should NOT trigger warning - no initialization)
extern int extern_var;

// 3. CONST/CONSTEXPR AT GLOBAL SCOPE
const int const_val1 = 42;
constexpr int constexpr_val1 = 100;
const double const_pi = 3.14159;

// 4. GLOBAL WITH INITIAL VALUE (equals style)
int global_with_init = 10;
