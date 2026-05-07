// RUN: %check_codelint %s codelint-init %t
// Test for type alias initialization
// Focus: using declarations

// 1. SIMPLE TYPE ALIASES
using IntAlias = int;
using DoubleAlias = double;
IntAlias alias1;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: variable is not initialized  [codelint-init]
DoubleAlias alias2;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable is not initialized  [codelint-init]

// 2. MULTIPLE ALIASES FOR SAME TYPE
using IntAlias2 = int;
IntAlias2 alias3;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: variable is not initialized  [codelint-init]

// 3. ALIAS IN LOCAL SCOPE
void test_alias_local() {
  using LocalInt = int;
  using LocalDouble = double;
  LocalInt local_alias1;
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: variable is not initialized  [codelint-init]
  LocalDouble local_alias2;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: using IntAlias = int;
// CHECK-FIXES: using DoubleAlias = double;
// CHECK-FIXES: IntAlias alias1{};
// CHECK-FIXES: DoubleAlias alias2{};
// CHECK-FIXES: using IntAlias2 = int;
// CHECK-FIXES: IntAlias2 alias3{};
// CHECK-FIXES: void test_alias_local() {
// CHECK-FIXES:   using LocalInt = int;
// CHECK-FIXES:   using LocalDouble = double;
// CHECK-FIXES:   LocalInt local_alias1{};
// CHECK-FIXES:   LocalDouble local_alias2{};
// CHECK-FIXES: }
