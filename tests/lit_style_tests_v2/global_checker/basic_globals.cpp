// RUN: %check_codelint %s codelint-global %t -- -std=c++17
// Test file: basic_globals.cpp
// Scenario: Basic global variable types
// Expected: 3 global variables detected

int global_int = 42;           // Basic int global
// CHECK-MESSAGES: :[@LINE]:5: warning: global variable 'global_int' detected  [codelint-global]
const int global_const = 100;  // Const int global
// CHECK-MESSAGES: :[@LINE]:11: warning: global variable 'global_const' detected  [codelint-global]
unsigned int global_uint = 5U; // Unsigned int with suffix
// CHECK-MESSAGES: :[@LINE]:14: warning: global variable 'global_uint' detected  [codelint-global]

int main() {
    return 0;
}
