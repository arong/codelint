// Test: Basic null pointer dereference patterns
// Expected warnings: codelint-null-deref

#include <cstddef>

// Test 1: Direct null assignment then dereference (ERROR)
void testDirectNull() {
  int* p = nullptr;
  *p = 10; // ERROR: dereference of null pointer
}

// Test 2: Dereference after reading null (ERROR)
void testReadNull() {
  int* p = nullptr;
  int x = *p; // ERROR: dereference of null pointer
}

// Test 3: Safe dereference with null check (OK)
void testSafeWithCheck() {
  int* p = nullptr;
  if (p != nullptr) {
    *p = 10; // OK: checked before dereference
  }
}

// Test 4: Dereference in else branch after null check (ERROR)
void testUnsafeInElse() {
  int* p = nullptr;
  if (p != nullptr) {
    // safe here
  } else {
    *p = 10; // ERROR: p is null here
  }
}

// Test 5: Dereference with implicit null check (ERROR)
void testImplicitNullCheck() {
  int* p = nullptr;
  if (p) {
    *p = 10; // OK
  } else {
    *p = 20; // ERROR: p is null in else branch
  }
}

// Test 6: Conditional assignment - may be null (WARNING)
int* maybeGetPointer(bool condition);

void testConditionalNull() {
  int* p = maybeGetPointer(true); // may return null
  *p = 10;                        // WARNING: potential dereference of null pointer
}

// Test 7: Member access on null pointer (ERROR)
struct Data {
  int value;
};

void testMemberAccessNull() {
  Data* d = nullptr;
  d->value = 10; // ERROR: dereference of null pointer
}

// Test 8: Arrow operator on null (ERROR)
void testArrowOperator() {
  Data* d = nullptr;
  int x = d->value; // ERROR: dereference of null pointer
}

// Test 9: Array subscript on null (ERROR)
void testArraySubscriptNull() {
  int* arr = nullptr;
  arr[0] = 10; // ERROR: dereference of null pointer
}

// Test 10: Reassignment to null then dereference (ERROR)
void testReassignNull() {
  int* p = new int(10);
  delete p;
  p = nullptr;
  *p = 20; // ERROR: dereference of null pointer
}

// Test 11: Pointer from function that may return null (WARNING)
int* getPointerMaybe();

void testFunctionMayReturnNull() {
  int* p = getPointerMaybe();
  *p = 10; // WARNING: potential dereference of null pointer
}

// Test 12: Pointer known to be non-null (OK)
int* getPointerNonNull();

void testFunctionReturnsNonNull() {
  int* p = getPointerNonNull();
  *p = 10; // OK: assumed non-null
}

// Test 13: Safe after assignment (OK)
void testSafeAfterAssignment() {
  int* p = nullptr;
  p = new int(10);
  *p = 20; // OK: assigned to non-null
  delete p;
}

// Test 14: Multiple pointers - one null one not (ERROR)
void testMultiplePointers() {
  int* p1 = nullptr;
  int* p2 = new int(10);
  *p1 = 5;  // ERROR: p1 is null
  *p2 = 15; // OK
  delete p2;
}
