// RUN: %codelint %s codelint-singleton %t
// Test file: false_positives.cpp (MERGED: 4 false positive test files)
// Scenario: Cases that should NOT be detected as singleton patterns
// Expected: 0 warnings (all are false positives)

// ============================================================================
// MERGED FROM: false_positive_pointer.cpp
// ============================================================================
// False Positive - Return Pointer (NOT a singleton)
// Returns pointer, not reference
class Resource {
public:
  static Resource* get() { // Returns pointer, not reference
    static Resource res;
    return &res;
  }
};

// ============================================================================
// MERGED FROM: false_positive_value.cpp
// ============================================================================
// False Positive - Return by Value (NOT a singleton)
// Returns by value, not by reference
class Factory {
public:
  static Factory create() { // Returns by value, not by reference
    return Factory();
  }
};

// ============================================================================
// MERGED FROM: false_positive_ref.cpp
// ============================================================================
// False Positive - Parameter Reference (NOT a singleton)
// Returns parameter reference, not static local
int& getRef(int& x) {
  return x; // Returns parameter reference, not static local
}

// ============================================================================
// MERGED FROM: false_positive_static.cpp
// ============================================================================
// False Positive - Static Local Variable (NOT a singleton)
// Return type is not a reference
void helper() {
  static int counter = 0; // Not a singleton, return type is not a reference
  counter++;
}
