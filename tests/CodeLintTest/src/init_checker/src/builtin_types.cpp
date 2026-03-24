// Test for builtin primitive types initialization
// Focus: char, float, double, bool (int types covered in integer.cpp)

// 1. CHAR TYPES
char c1;
unsigned char uc1;
signed char sc1;

// 2. FLOATING POINT TYPES
float f1;
double d1;
long double ld1;

// 3. BOOLEAN
bool b1;

// 4. STRING TYPES (const char* - pointer, std::string - non-builtin)
const char* str1;

// 5. SIGNED/UNSIGNED SHORT (distinct from integer.cpp)
short s1;
unsigned short us1;
signed short ss1;

void test_local_builtin() {
  char local_char;
  float local_float;
  double local_double;
  bool local_bool;
}
