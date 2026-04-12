// Test file: array_globals.cpp
// Scenario: Global array variables
// Expected: 3 global variables detected

int global_array[10];               // C-style array global - SHOULD detect
double values[5] = {1.0, 2.0, 3.0}; // C-style array with init - SHOULD detect
const char* strings[3];             // Pointer array global - SHOULD detect

int main() {
  return 0;
}
