// RUN: %check_codelint %s codelint-init %t -- -std=c++17
// Test for reference type initialization
// P1-2: Reference initialization style checks (only compilable code)

#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
void test_reference_initialization() {
  int value = 10;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int& ref1 = value; // Should suggest brace init: int& ref1{value}
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int& ref2{value};  // OK - already using brace init

  const int& cref1 = 42; // Should suggest brace init: const int& cref1{42}
// CHECK-MESSAGES: :[@LINE]:14: warning: variable should use '{}' syntax for initialization  [codelint-init]
  const int& cref2{100}; // OK - already using brace init
}

void test_reference_assignment_style() {
  int x = 5;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int y = 10;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]

  int& ref1 = x; // Should suggest brace init
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int& ref2 = y; // Should suggest brace init
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int& ref3{x};  // OK - already using brace init
}

void test_reference_in_struct() {
  struct RefStruct {
    int& ref;
    int value; // Should trigger warning: not initialized
// CHECK-MESSAGES: :[@LINE]:9: error: field is not initialized  [codelint-init]
  };

  int x = 5;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]
  RefStruct rs1{x, 10}; // OK
}

void test_reference_parameters(int& param) {
  int& local_ref = param; // Should suggest brace init
// CHECK-MESSAGES: :[@LINE]:8: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int& param_ref{param};  // OK - already using brace init
}

class ReferenceClass {
  int& member_ref;
  int value; // Should trigger warning: not initialized
// CHECK-MESSAGES: :[@LINE]:7: error: field is not initialized  [codelint-init]
// CHECK-MESSAGES: :[@LINE]:7: error: member variable 'value' is not initialized in constructor  [codelint-init]

public:
  ReferenceClass(int& r) : member_ref(r) {
  } // value not initialized in constructor
};

void test_rvalue_references() {
  int x = 10;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int&& rref1 = std::move(x); // Should suggest brace init
// CHECK-MESSAGES: :[@LINE]:9: warning: variable should use '{}' syntax for initialization  [codelint-init]
  int&& rref2{std::move(x)};  // OK - already using brace init
}

void ref_with_map() {
  std::unordered_map<std::string, uint32_t> table{
      {"debug", 1},
      {"info", 2},
  };
  auto& value{table["debug"]};
  std::cout << value;
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <iostream>
// CHECK-FIXES: #include <string>
// CHECK-FIXES: #include <unordered_map>
// CHECK-FIXES: #include <utility>
// CHECK-FIXES: void test_reference_initialization() {
// CHECK-FIXES:   int value{10};
// CHECK-FIXES:   int& ref1{value}; // Should suggest brace init: int& ref1{value}
// CHECK-FIXES:   int& ref2{value}; // OK - already using brace init
// CHECK-FIXES:   const int& cref1{42};  // Should suggest brace init: const int& cref1{42}
// CHECK-FIXES:   const int& cref2{100}; // OK - already using brace init
// CHECK-FIXES: }
// CHECK-FIXES: void test_reference_assignment_style() {
// CHECK-FIXES:   int x{5};
// CHECK-FIXES:   int y{10};
