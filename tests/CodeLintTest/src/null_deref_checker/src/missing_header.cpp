// Test: Null pointer dereference - edge cases
// This file intentionally has compilation errors to test error handling

#include <nonexistent_header.h> // This will cause compilation error

void testMissingHeader() {
  int* p = nullptr;
  *p = 10; // Should not produce codelint-null-deref due to compilation error
}
