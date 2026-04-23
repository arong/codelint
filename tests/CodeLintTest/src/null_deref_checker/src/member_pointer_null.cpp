// Test: Null pointer dereference for class member pointers
// Expected warnings: codelint-null-deref

#include <cstddef>

class Container {
public:
  int* dataPtr;
  int value;

  Container() : dataPtr(nullptr), value(0) {
  }

  // Test 1: Member pointer dereference (ERROR)
  void unsafeDeref() {
    *dataPtr = 10; // ERROR: dataPtr may be null
  }

  // Test 2: Safe member pointer dereference (OK)
  void safeDeref() {
    if (dataPtr != nullptr) {
      *dataPtr = 10; // OK: checked before dereference
    }
  }

  // Test 3: Arrow operator on member pointer (WARNING)
  struct Inner {
    int x;
  };
  Inner* innerPtr;

  void accessInner() {
    innerPtr->x = 5; // WARNING: may be null
  }

  // Test 4: Array subscript on member pointer (WARNING)
  void accessArray() {
    dataPtr[0] = 1; // WARNING: may be null
  }
};

// Test 5: Pointer to member dereference (WARNING)
void testPointerToMember() {
  Container* container = new Container();
  *container->dataPtr = 10; // WARNING: dataPtr may be null
  delete container;
}

// Test 6: Chain of pointer dereference (WARNING)
struct Node {
  Node* next;
  int data;
};

void testChainDeref() {
  Node* head = nullptr;
  head->next = nullptr; // ERROR: head is null
}

// Test 7: Deep nested member access (WARNING)
struct Outer {
  Container* container;
};

void testNestedMemberAccess() {
  Outer outer;
  outer.container = nullptr;
  *outer.container->dataPtr = 10; // WARNING: container may be null
}

// Test 8: This pointer is always non-null (OK)
class SelfReferential {
public:
  int data;
  void setData(int x) {
    this->data = x; // OK: this is never null in well-defined code
  }
};

// Test 9: Pointer member in constructor (WARNING)
class LazyInit {
public:
  int* ptr;
  LazyInit() : ptr(nullptr) {
  }

  void init() {
    ptr = new int(10);
  }

  void use() {
    *ptr = 20; // WARNING: may be null if init() not called
  }
};

// Test 10: Multiple member pointers (ERROR in one)
class MultiPointer {
public:
  int* p1;
  int* p2;

  MultiPointer() : p1(nullptr), p2(nullptr) {
  }

  void setup() {
    p1 = new int(10);
    // p2 intentionally not initialized
  }

  void use() {
    *p1 = 20; // OK after setup
    *p2 = 30; // ERROR: p2 is null
  }
};

// Test 11: Pointer member through reference (WARNING)
void testThroughReference(Container& c) {
  *c.dataPtr = 10; // WARNING: dataPtr may be null
}

// Test 12: Const member pointer (WARNING)
class ConstContainer {
public:
  int* const constPtr; // Must be initialized in constructor

  ConstContainer(int* p) : constPtr(p) {
  }

  void use() {
    *constPtr = 10; // WARNING: depends on constructor argument
  }
};
