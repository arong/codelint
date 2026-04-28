// RUN: %check_codelint %s codelint-init %t -- -std=c++17
// Test for builtin primitive types initialization
// Focus: char, float, double, bool (int types covered in integer.cpp)

// 1. CHAR TYPES
char c1;
// CHECK-MESSAGES: :[@LINE]:6: error: variable is not initialized  [codelint-init]
unsigned char uc1;
// CHECK-MESSAGES: :[@LINE]:15: error: variable is not initialized  [codelint-init]
signed char sc1;
// CHECK-MESSAGES: :[@LINE]:13: error: variable is not initialized  [codelint-init]

// 2. FLOATING POINT TYPES
float f1;
// CHECK-MESSAGES: :[@LINE]:7: error: variable is not initialized  [codelint-init]
double d1;
// CHECK-MESSAGES: :[@LINE]:8: error: variable is not initialized  [codelint-init]
long double ld1;
// CHECK-MESSAGES: :[@LINE]:13: error: variable is not initialized  [codelint-init]

// 3. BOOLEAN
bool b1;
// CHECK-MESSAGES: :[@LINE]:6: error: variable is not initialized  [codelint-init]

// 4. STRING TYPES (const char* - pointer, std::string - non-builtin)
const char* str1;
// CHECK-MESSAGES: :[@LINE]:13: error: variable is not initialized  [codelint-init]

// 5. SIGNED/UNSIGNED SHORT (distinct from integer.cpp)
short s1;
// CHECK-MESSAGES: :[@LINE]:7: error: variable is not initialized  [codelint-init]
unsigned short us1;
// CHECK-MESSAGES: :[@LINE]:16: error: variable is not initialized  [codelint-init]
signed short ss1;
// CHECK-MESSAGES: :[@LINE]:14: error: variable is not initialized  [codelint-init]

void test_local_builtin() {
  char local_char;
// CHECK-MESSAGES: :[@LINE]:8: error: variable is not initialized  [codelint-init]
  float local_float;
// CHECK-MESSAGES: :[@LINE]:9: error: variable is not initialized  [codelint-init]
  double local_double;
// CHECK-MESSAGES: :[@LINE]:10: error: variable is not initialized  [codelint-init]
  bool local_bool;
// CHECK-MESSAGES: :[@LINE]:8: error: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: char c1{};
// CHECK-FIXES: unsigned char uc1{};
// CHECK-FIXES: signed char sc1{};
// CHECK-FIXES: float f1{};
// CHECK-FIXES: double d1{};
// CHECK-FIXES: long double ld1{};
// CHECK-FIXES: bool b1{};
// CHECK-FIXES: const char* str1{};
