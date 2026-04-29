// RUN: %codelint %s codelint-init %t
// Test for global/static variable initialization
// Focus: global scope, static, extern

// 1. GLOBAL VARIABLES
int global_var1;
// CHECK-MESSAGES: :6:5: error: variable is not initialized  [codelint-init]
static int static_global1;
// CHECK-MESSAGES: :8:12: error: variable is not initialized  [codelint-init]

// 2. EXTERN DECLARATION (should NOT trigger warning - no initialization)
extern int extern_var;

// 3. CONST/CONSTEXPR AT GLOBAL SCOPE
const int const_val1 = 42;
// CHECK-MESSAGES: :15:11: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
constexpr int constexpr_val1 = 100;
// CHECK-MESSAGES: :17:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
const double const_pi = 3.14159;
// CHECK-MESSAGES: :19:14: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

// 4. GLOBAL WITH INITIAL VALUE (equals style)
int global_with_init = 10;
// CHECK-MESSAGES: :23:5: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

// === Expected Fixed Output ===
// CHECK-FIXES: int global_var1{};
// CHECK-FIXES: static int static_global1{};
// CHECK-FIXES: extern int extern_var;
// CHECK-FIXES: const int const_val1{42};
// CHECK-FIXES: constexpr int constexpr_val1{100};
// CHECK-FIXES: const double const_pi{3.14159};
// CHECK-FIXES: int global_with_init{10};
