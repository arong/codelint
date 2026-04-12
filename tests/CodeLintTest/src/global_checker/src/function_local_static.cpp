// Test file: function_local_static.cpp
// Scenario: Function-local static variables (NOT globals)
// Expected: 0 global variables detected (false positive test)

void counter() {
  static int call_count = 0; // Function-local static - NOT a global
  call_count++;
}

void helper() {
  static double cached_value = 3.14; // Another function-local static
}

int main() {
  counter();
  helper();
  return 0;
}
