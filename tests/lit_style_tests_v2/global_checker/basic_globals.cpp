// RUN: %codelint %s codelint-global %t
// Test file: basic_globals.cpp
// Scenario: Basic global variable types
// Expected: 3 global variables detected

int global_int = 42;           // Basic int global
// CHECK-MESSAGES: :6:5: warning: global variable 'global_int' detected  [codelint-global]
const int global_const = 100;  // Const int global
// CHECK-MESSAGES: :8:11: warning: global variable 'global_const' detected  [codelint-global]
unsigned int global_uint = 5U; // Unsigned int with suffix
// CHECK-MESSAGES: :10:14: warning: global variable 'global_uint' detected  [codelint-global]

int main() {
    return 0;
}
