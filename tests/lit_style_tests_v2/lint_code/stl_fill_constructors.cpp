// RUN: %codelint %s codelint-lint-code %t
// Test for all STL container constructor semantics
// Critical: Many STL containers have fill constructors that change semantics with brace init

#include <deque>
#include <forward_list>
#include <list>
#include <string>
#include <vector>

// =============================================================================
// std::vector fill constructors - MUST NOT WARN
// =============================================================================

void test_vector_fill() {
  std::vector<int> v1(5, 0);
  std::vector<int> v2(10, 42);
  std::vector<double> v3(3, 3.14);
}

// =============================================================================
// std::deque fill constructors - MUST NOT WARN
// =============================================================================

void test_deque_fill() {
  std::deque<int> d1(5, 0);
  std::deque<int> d2(10, 42);
  std::deque<std::string> d3(3, "test");
}

// =============================================================================
// std::list fill constructors - MUST NOT WARN
// =============================================================================

void test_list_fill() {
  std::list<int> l1(5, 0);
  std::list<int> l2(10, 42);
  std::list<double> l3(3, 3.14);
}

// =============================================================================
// std::forward_list fill constructors - MUST NOT WARN
// =============================================================================

void test_forward_list_fill() {
  std::forward_list<int> fl1(5, 0);
  std::forward_list<int> fl2(10, 42);
  std::forward_list<std::string> fl3(3, "test");
}

// =============================================================================
// std::string fill constructors - MUST NOT WARN
// =============================================================================

void test_string_fill() {
  std::string s1(5, 'a');
  std::string s2(10, 'x');
  std::string s3(3, '\n');
}

// =============================================================================
// std::wstring fill constructors - MUST NOT WARN
// =============================================================================

void test_wstring_fill() {
  std::wstring ws1(5, L'a');
  std::wstring ws2(10, L'x');
}

// =============================================================================
// Single argument constructors that change semantics
// =============================================================================

void test_single_arg_fill() {
  // std::vector(1) creates 1 default-initialized element
  // brace init {1} creates 1 element with value 1 - DIFFERENT!
  std::vector<int> v1(1);
  std::deque<int> d1(1);
  std::list<int> l1(1);
}

// =============================================================================
// Iterator range constructors - MUST NOT WARN (special handling)
// =============================================================================

void test_iterator_range() {
  std::vector<int> source{1, 2, 3};

  std::vector<int> v1(source.begin(), source.end());
  std::deque<int> d1(source.begin(), source.end());
  std::list<int> l1(source.begin(), source.end());
  std::string s1(source.begin(), source.end());
}

// =============================================================================
// Cases that SHOULD WARN (safe to convert)
// =============================================================================

void test_safe_to_convert() {
  std::string s1 = "hello";
  // CHECK-MESSAGES: :100:15: warning: variable should use '{}' syntax for initialization
  // [codelint-lint-code]
  std::string s2("world");
  // CHECK-MESSAGES: :102:15: warning: variable should use '{}' syntax for initialization
  // [codelint-lint-code]
  std::wstring ws1 = L"hello";
  // CHECK-MESSAGES: :104:16: warning: variable should use '{}' syntax for initialization
  // [codelint-lint-code]
}

// =============================================================================
// Already using brace init correctly - MUST NOT WARN
// =============================================================================

void test_already_brace() {
  std::vector<int> v1{1, 2, 3};
  std::vector<int> v2{};
  std::deque<int> d1{1, 2, 3};
  std::list<int> l1{1, 2, 3};
  std::forward_list<int> fl1{1, 2, 3};
  std::string s1{"hello"};
  std::wstring ws1{L"hello"};
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <deque>
// CHECK-FIXES: #include <forward_list>
// CHECK-FIXES: #include <list>
// CHECK-FIXES: #include <string>
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: void test_vector_fill() {
// CHECK-FIXES:   std::vector<int> v1(5, 0);
// CHECK-FIXES:   std::vector<int> v2(10, 42);
// CHECK-FIXES:   std::vector<double> v3(3, 3.14);
// CHECK-FIXES: }
