// RUN: %codelint %s codelint-lint-code %t
// Test for brace initialization style transformations
// Focus: = → {} conversion, = {} → {} removal, auto brace → equals

// 1. EQUALS TO BRACE INITIALIZATION
int a = 10;              // Should suggest: int a{10}
// CHECK-MESSAGES: :6:5: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
double b = 3.14;         // Should suggest: double b{3.14}
// CHECK-MESSAGES: :8:8: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
std::string s = "hello"; // Should suggest: std::string s{"hello"}
// CHECK-MESSAGES: :10:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

// 2. EQUALS BRACE TO DIRECT BRACE
int c = {1}; // Should suggest: int c{1}
// CHECK-MESSAGES: :14:5: warning: initializer should use '{}' syntax instead of '= {}'  [codelint-lint-code]
int d = {};  // Should suggest: int d{}
// CHECK-MESSAGES: :16:5: warning: initializer should use '{}' syntax instead of '= {}'  [codelint-lint-code]

// 3. AUTO BRACE TO EQUALS (opposite direction)
auto x{42};         // Should suggest: auto x = 42
// CHECK-MESSAGES: :20:5: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]
auto* p{&a};        // Should suggest: auto *p = &a
// CHECK-MESSAGES: :22:6: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]
const auto* cp{&a}; // Should suggest: const auto *cp = &a
// CHECK-MESSAGES: :24:11: warning: auto type should use '=' assignment instead of brace initialization  [codelint-lint-code]

// 4. UNSIGNED SUFFIX
unsigned u = 100;  // Should suggest: unsigned u{100U}
// CHECK-MESSAGES: :28:9: warning: unsigned integer literal should have 'U' suffix  [codelint-lint-code]
uint64_t big = 42; // Should suggest: uint64_t big{42UL}
// CHECK-MESSAGES: :30:11: warning: unsigned integer literal should have 'UL' suffix  [codelint-lint-code]

// 5. CALL INIT TO BRACE
std::string str("world"); // Should suggest: std::string str{"world"}
// CHECK-MESSAGES: :34:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

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
