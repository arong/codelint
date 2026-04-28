// RUN: %check_codelint %s codelint-lint-code %t -- -std=c++17
// Test for brace initialization style transformations
// Focus: = → {} conversion, = {} → {} removal, auto brace → equals

// 1. EQUALS TO BRACE INITIALIZATION
int a = 10;              // Should suggest: int a{10}
// CHECK-MESSAGES: :[@LINE]:5: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
double b = 3.14;         // Should suggest: double b{3.14}
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
std::string s = "hello"; // Should suggest: std::string s{"hello"}
// CHECK-MESSAGES: :[@LINE]:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

// 2. EQUALS BRACE TO DIRECT BRACE
int c = {1}; // Should suggest: int c{1}
// CHECK-MESSAGES: :[@LINE]:5: warning: initializer should use '{}' syntax instead of '= {}'  [codelint-lint-code]
int d = {};  // Should suggest: int d{}
// CHECK-MESSAGES: :[@LINE]:5: warning: initializer should use '{}' syntax instead of '= {}'  [codelint-lint-code]

// 3. AUTO BRACE TO EQUALS (opposite direction)
auto x{42};         // Should suggest: auto x = 42
// CHECK-MESSAGES: :[@LINE]:5: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]
auto* p{&a};        // Should suggest: auto *p = &a
// CHECK-MESSAGES: :[@LINE]:6: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]
const auto* cp{&a}; // Should suggest: const auto *cp = &a
// CHECK-MESSAGES: :[@LINE]:11: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]

// 4. UNSIGNED SUFFIX
unsigned u = 100;  // Should suggest: unsigned u{100U}
// CHECK-MESSAGES: :[@LINE]:9: warning: unsigned integer literal should have 'U' suffix  [codelint-lint-code]
uint64_t big = 42; // Should suggest: uint64_t big{42UL}
// CHECK-MESSAGES: :[@LINE]:11: warning: unsigned integer literal should have 'UL' suffix  [codelint-lint-code]

// 5. CALL INIT TO BRACE
std::string str("world"); // Should suggest: std::string str{"world"}
// CHECK-MESSAGES: :[@LINE]:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

// 6. VALID CODE (no warnings)
int valid{10};         // Already correct brace init
auto valid2 = 42;      // Already correct auto equals
unsigned valid3{100U}; // Already has suffix

// === Expected Fixed Output ===
// CHECK-FIXES: #include <cstddef>
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: #include <string>
// CHECK-FIXES: int a{10};
// CHECK-FIXES: double b{3.14};
// CHECK-FIXES: std::string s{"hello"};
// CHECK-FIXES: int c{1};
// CHECK-FIXES: int d{};
// CHECK-FIXES: auto x = 42;
// CHECK-FIXES: auto* p = &a;
// CHECK-FIXES: const auto* cp = &a;
// CHECK-FIXES: unsigned u{100U};
// CHECK-FIXES: uint64_t big{42UL};
// CHECK-FIXES: std::string str{"world"};
