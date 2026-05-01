// RUN: %codelint %s codelint-global %t

#include <string>
#include <vector>

// Basic int types
int global_int = 42;
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'global_int' detected  [codelint-global]
const int global_const = 100;
// CHECK-MESSAGES: :[@LINE-1]:11: warning: global variable 'global_const' detected [codelint-global]
unsigned int global_uint = 5U;
// CHECK-MESSAGES: :[@LINE-1]:14: warning: global variable 'global_uint' detected  [codelint-global]

// Various type globals
float global_float = 3.14f;
// CHECK-MESSAGES: :[@LINE-1]:7: warning: global variable 'global_float' detected  [codelint-global]
double global_double = 2.718;
// CHECK-MESSAGES: :[@LINE-1]:8: warning: global variable 'global_double' detected [codelint-global]
char global_char = 'A';
// CHECK-MESSAGES: :[@LINE-1]:6: warning: global variable 'global_char' detected  [codelint-global]
bool global_bool = true;
// CHECK-MESSAGES: :[@LINE-1]:6: warning: global variable 'global_bool' detected  [codelint-global]
std::vector<int> global_vec;
// CHECK-MESSAGES: :[@LINE-1]:18: warning: global variable 'global_vec' detected  [codelint-global]
std::string global_str = "hello";
// CHECK-MESSAGES: :[@LINE-1]:13: warning: global variable 'global_str' detected  [codelint-global]

// Const and constexpr globals
const int const_val = 100;
// CHECK-MESSAGES: :[@LINE-1]:11: warning: global variable 'const_val' detected  [codelint-global]
constexpr int constexpr_val = 200;
const std::string const_str = "const";
// CHECK-MESSAGES: :[@LINE-1]:19: warning: global variable 'const_str' detected  [codelint-global]

// Static global variables
static int static_int = 10;
// CHECK-MESSAGES: :[@LINE-1]:12: warning: global variable 'static_int' detected  [codelint-global]
static std::string static_str = "test";
// CHECK-MESSAGES: :[@LINE-1]:20: warning: global variable 'static_str' detected  [codelint-global]

// Global class/struct instances
struct MyStruct {
  int x;
};
class MyClass {
public:
  int value;
};

MyStruct global_struct;
// CHECK-MESSAGES: :[@LINE-1]:10: warning: global variable 'global_struct' detected
// [codelint-global]
MyClass global_class;
// CHECK-MESSAGES: :[@LINE-1]:9: warning: global variable 'global_class' detected  [codelint-global]

// Global array variables
int global_array[10];
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'global_array' detected  [codelint-global]
double values[5] = {1.0, 2.0, 3.0};
// CHECK-MESSAGES: :[@LINE-1]:8: warning: global variable 'values' detected  [codelint-global]
const char* strings[3];
// CHECK-MESSAGES: :[@LINE-1]:13: warning: global variable 'strings' detected  [codelint-global]

int main() {
  return 0;
}
