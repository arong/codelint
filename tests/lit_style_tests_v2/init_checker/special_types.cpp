// RUN: %check_codelint %s codelint-init %t
// Test for special/cstdint types initialization
// Focus: size_t, ptrdiff_t, wchar_t, char16_t, char32_t

#include <cstddef>
#include <cstdint>

// 1. SIZE TYPES
size_t sz1;
// CHECK-MESSAGES: :[[@LINE-1]]:8: error: variable is not initialized  [codelint-init]
ptrdiff_t pt1;
// CHECK-MESSAGES: :[[@LINE-1]]:11: error: variable is not initialized  [codelint-init]

// 2. WIDE CHAR TYPES
wchar_t wc1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
char16_t char16_1;
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]
char32_t char32_1;
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]

// 3. FIXED-WIDTH INTEGER TYPES (complement to integer.cpp)
int16_t i16;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
uint16_t ui16;
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]
int32_t i32;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
uint32_t ui32;
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]
int64_t i64;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
uint64_t ui64;
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]

void test_local_special() {
  size_t local_sz;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: error: variable is not initialized  [codelint-init]
  ptrdiff_t local_pt;
  // CHECK-MESSAGES: :[[@LINE-1]]:13: error: variable is not initialized  [codelint-init]
  wchar_t local_wc;
  // CHECK-MESSAGES: :[[@LINE-1]]:11: error: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <cstddef>
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: size_t sz1{};
// CHECK-FIXES: ptrdiff_t pt1{};
// CHECK-FIXES: wchar_t wc1{};
// CHECK-FIXES: char16_t char16_1{};
// CHECK-FIXES: char32_t char32_1{};
// CHECK-FIXES: int16_t i16{};
// CHECK-FIXES: uint16_t ui16{};
// CHECK-FIXES: int32_t i32{};
// CHECK-FIXES: uint32_t ui32{};
