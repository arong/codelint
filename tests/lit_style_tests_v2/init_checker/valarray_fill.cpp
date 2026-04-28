// RUN: %check_codelint %s codelint-init %t -- -std=c++17
// Test for std::valarray constructor semantics
// Critical: std::valarray has fill constructor with reversed parameter order

#include <valarray>

// =============================================================================
// std::valarray fill constructors - MUST NOT WARN
// Note: Parameter order is reversed compared to other containers!
// valarray(const T& val, size_type count) - value first, count second
// =============================================================================

void test_valarray_fill() {
  std::valarray<int> v1(5, 10);
  std::valarray<int> v2(42, 100);
  std::valarray<double> v3(3.14, 50);
}

// =============================================================================
// Single argument constructor that changes semantics
// valarray(1) creates valarray with 1 element of value 1
// brace init {1} creates valarray with 1 element of value 1 - SAME!
// But valarray(5, 10) vs {5, 10} are DIFFERENT
// =============================================================================

void test_valarray_single_arg() {
  std::valarray<int> v1(5);
  std::valarray<int> v2(10);
}

// =============================================================================
// Already using brace init correctly - MUST NOT WARN
// =============================================================================

void test_valarray_brace() {
  std::valarray<int> v1{1, 2, 3};
  std::valarray<int> v2{};
  std::valarray<double> v3{1.1, 2.2, 3.3};
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <valarray>
// CHECK-FIXES: void test_valarray_fill() {
// CHECK-FIXES:   std::valarray<int> v1(5, 10);
// CHECK-FIXES:   std::valarray<int> v2(42, 100);
// CHECK-FIXES:   std::valarray<double> v3(3.14, 50);
// CHECK-FIXES: }
