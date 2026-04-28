// RUN: %check_codelint %s codelint-global %t -- -std=c++17
// Test file: inline_globals.cpp
// Scenario: C++17 inline variables
// Expected: 1 global variable detected

inline int inline_var = 42;
// CHECK-MESSAGES: :[@LINE]:12: warning: global variable 'inline_var' detected  [codelint-global]

int main() {
    return 0;
}
