// RUN: %codelint %s codelint-global %t
// Test file: typed_globals.cpp
// Scenario: Various type global variables
// Expected: 6 global variables detected

#include <vector>
#include <string>

float global_float = 3.14f;       // float
// CHECK-MESSAGES: :9:7: warning: global variable 'global_float' detected  [codelint-global]
double global_double = 2.718;     // double
// CHECK-MESSAGES: :11:8: warning: global variable 'global_double' detected  [codelint-global]
char global_char = 'A';           // char
// CHECK-MESSAGES: :13:6: warning: global variable 'global_char' detected  [codelint-global]
bool global_bool = true;          // bool
// CHECK-MESSAGES: :15:6: warning: global variable 'global_bool' detected  [codelint-global]
std::vector<int> global_vec;      // std::vector
// CHECK-MESSAGES: :17:18: warning: global variable 'global_vec' detected  [codelint-global]
std::string global_str = "hello"; // std::string
// CHECK-MESSAGES: :19:13: warning: global variable 'global_str' detected  [codelint-global]

int main() {
    return 0;
}
