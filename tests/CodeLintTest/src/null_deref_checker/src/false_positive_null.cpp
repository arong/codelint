// Test: False positive cases - these should NOT produce warnings
// Expected: No codelint-null-deref warnings

#include <cassert>
#include <cstddef>

// Test 1: Pointer checked before dereference (OK)
void testCheckedDeref(int* p) {
  if (p != nullptr) {
    *p = 10; // OK: checked
  }
}

// Test 2: Pointer checked with implicit bool (OK)
void testImplicitCheck(int* p) {
  if (p) {
    *p = 10; // OK: implicit check
  }
}

// Test 3: Assert before dereference (OK)
void testAssertBeforeDeref(int* p) {
  assert(p != nullptr);
  *p = 10; // OK: asserted non-null
}

// Test 4: Early return on null (OK)
void testEarlyReturnNull(int* p) {
  if (!p)
    return;
  *p = 10; // OK: returned if null
}

// Test 5: Assignment before dereference (OK)
void testAssignBeforeDeref() {
  int* p = nullptr;
  p = new int(10);
  *p = 20; // OK: assigned before use
  delete p;
}

// Test 6: New expression result (OK)
void testNewResult() {
  int* p = new int(10);
  *p = 20; // OK: new returns non-null (or throws)
  delete p;
}

// Test 7: Address of operator (OK)
void testAddressOf() {
  int x = 10;
  int* p = &x;
  *p = 20; // OK: address-of always valid
}

// Test 8: Reference to pointer (OK)
void testReferenceParam(int*& p) {
  if (p != nullptr) {
    *p = 10; // OK: checked
  }
}

// Test 9: Loop with guaranteed initialization (OK)
void testLoopGuaranteedInit() {
  int* p = nullptr;
  for (int i = 0; i < 10; ++i) {
    p = new int(i);
    *p = i * 2; // OK: assigned in loop
    delete p;
  }
}

// Test 10: Switch with default initialization (OK)
void testSwitchWithDefault(int x) {
  int* p = nullptr;
  switch (x) {
  case 1:
    p = new int(10);
    break;
  default:
    p = new int(20); // Always assigned
    break;
  }
  *p = 30; // OK: assigned in all paths
  delete p;
}

// Test 11: Ternary with non-null result (OK)
void testTernaryNonNull(bool cond) {
  int a = 10, b = 20;
  int* p = cond ? &a : &b;
  *p = 30; // OK: always points to valid memory
}

// Test 12: Function with non-null return attribute
struct Data {
  int x;
};
Data* getData() __attribute__((returns_nonnull));

void testNonNullAttribute() {
  Data* d = getData();
  d->x = 10; // OK: marked as returns_nonnull
}

// Test 13: Array to pointer decay (OK)
void testArrayDecay() {
  int arr[5] = {1, 2, 3, 4, 5};
  int* p = arr;
  *p = 10; // OK: points to array
}

// Test 14: String literal (OK)
void testStringLiteral() {
  const char* s = "hello";
  char c = s[0]; // OK: string literal is not null
}

// Test 15: Static local variable address (OK)
void testStaticLocal() {
  static int x = 10;
  int* p = &x;
  *p = 20; // OK: static has permanent address
}

// Test 16: Global variable address (OK)
int globalVar = 10;

void testGlobalAddress() {
  int* p = &globalVar;
  *p = 20; // OK: global has permanent address
}

// Test 17: Member access through object (OK)
struct Container {
  int data;
  int* ptr;
  Container() : ptr(&data) {
  }
};

void testMemberAccess() {
  Container c;
  *c.ptr = 10; // OK: initialized in constructor
}

// Test 18: Smart pointer dereference (OK for unique_ptr, shared_ptr)
#include <memory>
void testSmartPointer() {
  auto p = std::make_unique<int>(10);
  *p = 20; // OK: unique_ptr guarantees non-null after make_unique
}

// Test 19: Placement new (OK)
void testPlacementNew() {
  alignas(int) char buffer[sizeof(int)];
  int* p = new (buffer) int(10);
  *p = 20; // OK: placement new into valid buffer
}

// Test 20: Null check with logical OR short-circuit (OK)
void testLogicalOrShortCircuit(int* p) {
  if (p != nullptr || checkOtherCondition()) {
    // p may still be null here, don't dereference
  }
  if (p != nullptr) {
    *p = 10; // OK: checked
  }
}

bool checkOtherCondition();

// Test 21: While loop that always assigns (OK)
void testWhileAlwaysAssigns() {
  int* p = nullptr;
  bool done = false;
  while (!done) {
    p = new int(10);
    done = true;
  }
  *p = 20; // OK: assigned in loop
  delete p;
}

// Test 22: For loop with guaranteed init (OK)
void testForGuaranteedInit() {
  int* p = nullptr;
  for (int i = 0; i < 1; ++i) {
    p = new int(i);
  }
  if (p != nullptr) {
    *p = 20; // OK: checked
    delete p;
  }
}

// Test 23: Try-catch with guaranteed init in try (OK)
void testTryCatchInit() {
  int* p = nullptr;
  try {
    p = new int(10);
    *p = 20; // OK: assigned before use
    delete p;
  } catch (...) {
    // p may not be initialized here
  }
}

// Test 24: Const pointer that's initialized (OK)
void testConstPointer() {
  int x = 10;
  int* const p = &x;
  *p = 20; // OK: const pointer initialized with valid address
}

// Test 25: Member function this pointer (always OK)
class TestClass {
public:
  int data;
  void setData(int x) {
    this->data = x; // OK: this is never null
  }
};
