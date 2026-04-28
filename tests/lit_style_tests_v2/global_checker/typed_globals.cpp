// RUN: %check_codelint %s codelint-global %t -- -std=c++17
// Test file: typed_globals.cpp
// Scenario: Various type global variables
// Expected: 6 global variables detected

#include <vector>
#include <string>

float global_float = 3.14f;       // float
// CHECK-MESSAGES: :[@LINE]:7: warning: global variable 'global_float' detected  [codelint-global]
double global_double = 2.718;     // double
// CHECK-MESSAGES: :[@LINE]:8: warning: global variable 'global_double' detected  [codelint-global]
char global_char = 'A';           // char
// CHECK-MESSAGES: :[@LINE]:6: warning: global variable 'global_char' detected  [codelint-global]
bool global_bool = true;          // bool
// CHECK-MESSAGES: :[@LINE]:6: warning: global variable 'global_bool' detected  [codelint-global]
std::vector<int> global_vec;      // std::vector
// CHECK-MESSAGES: :[@LINE]:18: warning: global variable 'global_vec' detected  [codelint-global]
std::string global_str = "hello"; // std::string
// CHECK-MESSAGES: :[@LINE]:13: warning: global variable 'global_str' detected  [codelint-global]

int main() {
    return 0;
}
