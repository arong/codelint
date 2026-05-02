// RUN: %check_codelint %s codelint-lint-code %t
// Test for brace initialization style transformations
// Focus: = → {} conversion, = {} → {} removal, auto brace → equals

#include <cstddef>
#include <cstdint>
#include <string>

// 1. EQUALS TO BRACE INITIALIZATION
int a = 10; // Should suggest: int a{10}
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
double b = 3.14; // Should suggest: double b{3.14}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
std::string s = "hello"; // Should suggest: std::string s{"hello"}
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

// 2. EQUALS BRACE TO DIRECT BRACE
int c = {1}; // Should suggest: int c{1}
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: initializer should use '{}' syntax instead of '= {}'
// [codelint-lint-code]
int d = {}; // Should suggest: int d{}
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: initializer should use '{}' syntax instead of '= {}'
// [codelint-lint-code]

// 3. AUTO BRACE TO EQUALS (opposite direction)
auto x{42}; // Should suggest: auto x = 42
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]
auto* p{&a}; // Should suggest: auto *p = &a
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]
const auto* cp{&a}; // Should suggest: const auto *cp = &a
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]

// 4. UNSIGNED SUFFIX
unsigned u = 100; // Should suggest: unsigned u{100U}
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
uint64_t big = 42; // Should suggest: uint64_t big{42UL}
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

// 5. CALL INIT TO BRACE
std::string str("world"); // Should suggest: std::string str{"world"}
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

// 6. VALID CODE (no warnings)
int valid{10};         // Already correct brace init
auto valid2 = 42;      // Already correct auto equals
unsigned valid3{100U}; // Already has suffix
