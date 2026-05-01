// RUN: %codelint %s codelint-global %t
// Test file: edge_cases.cpp (MERGED: function_local_static + class_member)
// Scenario: Cases that should NOT be detected as global variables
// Expected: 0 global variables detected (all are edge cases / false positive tests)

// ============================================================================
// MERGED FROM: function_local_static.cpp
// Function-local static variables should NOT be detected
// ============================================================================
void counter() {
  static int call_count = 0; // Function-local static - NOT a global
  call_count++;
}

void helper() {
  static double cached_value = 3.14; // Another function-local static
}

// ============================================================================
// MERGED FROM: class_member.cpp
// Class/struct member variables should NOT be detected
// ============================================================================
class MyClass {
public:
  int public_member; // Class member - NOT a global

private:
  double private_member;    // Private member - NOT a global
  static int static_member; // Static class member - NOT a global
};

struct MyStruct {
  int field; // Struct field - NOT a global
};

int main() {
  counter();
  helper();
  return 0;
}
