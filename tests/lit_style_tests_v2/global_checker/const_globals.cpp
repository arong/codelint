// RUN: %check_codelint %s codelint-global %t -- -std=c++17
// Test file: const_globals.cpp
// Scenario: Const and constexpr globals
// Expected: 3 global variables detected

#include <string>

const int const_val = 100;
// CHECK-MESSAGES: :[@LINE]:11: warning: global variable 'const_val' detected  [codelint-global]
constexpr int constexpr_val = 200;
const std::string const_str = "const";
// CHECK-MESSAGES: :[@LINE]:19: warning: global variable 'const_str' detected  [codelint-global]

int main() {
    return 0;
}
