// Test for special/cstdint types initialization
// Focus: size_t, ptrdiff_t, wchar_t, char16_t, char32_t

#include <cstddef>
#include <cstdint>

// 1. SIZE TYPES
size_t sz1{};
ptrdiff_t pt1{};

// 2. WIDE CHAR TYPES
wchar_t wc1{};
char16_t char16_1{};
char32_t char32_1{};

// 3. FIXED-WIDTH INTEGER TYPES (complement to integer.cpp)
int16_t i16{};
uint16_t ui16{};
int32_t i32{};
uint32_t ui32{};
int64_t i64{};
uint64_t ui64{};

void test_local_special() {
  size_t local_sz{};
  ptrdiff_t local_pt{};
  wchar_t local_wc{};
}