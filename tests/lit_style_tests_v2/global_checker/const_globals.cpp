// RUN: %codelint %s codelint-global %t
// Test file: const_globals.cpp
// Scenario: Const and constexpr globals
// Expected: 3 global variables detected

#include <string>

const int const_val = 100;
// CHECK-MESSAGES: :8:11: warning: global variable 'const_val' detected  [codelint-global]
constexpr int constexpr_val = 200;
const std::string const_str = "const";
// CHECK-MESSAGES: :11:19: warning: global variable 'const_str' detected  [codelint-global]

int main() {
    return 0;
}
