// RUN: %codelint %s codelint-global %t
// Test file: array_globals.cpp
// Scenario: Global array variables
// Expected: 3 global variables detected

int global_array[10];               // C-style array global - SHOULD detect
// CHECK-MESSAGES: :6:5: warning: global variable 'global_array' detected  [codelint-global]
double values[5] = {1.0, 2.0, 3.0}; // C-style array with init - SHOULD detect
// CHECK-MESSAGES: :8:8: warning: global variable 'values' detected  [codelint-global]
const char* strings[3];             // Pointer array global - SHOULD detect
// CHECK-MESSAGES: :10:13: warning: global variable 'strings' detected  [codelint-global]

int main() {
  return 0;
}
