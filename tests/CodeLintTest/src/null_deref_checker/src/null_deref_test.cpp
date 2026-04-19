#include <cassert>

int* getPointer() {
  return nullptr;
}

void testNullDeref1() {
  int* p = nullptr;
  *p = 10;
}

void testNullDeref2() {
  int* p = nullptr;
  int x = *p;
}

void testNullDeref3() {
  int* p = nullptr;
  if (p != nullptr) {
    *p = 10;
  }
}

void testNullDeref4() {
  int* p = nullptr;
  if (p) {
    *p = 10;
  } else {
    *p = 20;
  }
}

void testNullDeref5() {
  int* p = getPointer();
  *p = 10;
}

void testNoDeref() {
  int* p = nullptr;
  p = new int(10);
  *p = 20;
}

void testSafeDeref() {
  int* p = nullptr;
  if (p != nullptr) {
    *p = 10;
  }
}

class TestClass {
public:
  int value;
};

void testMemberDeref() {
  TestClass* obj = nullptr;
  obj->value = 10;
}

void testArrowOperator() {
  TestClass* obj = nullptr;
  int x = obj->value;
}

void testArraySubscript() {
  int* arr = nullptr;
  arr[0] = 10;
}

void testAssignNullThenDeref() {
  int* p = new int(10);
  p = nullptr;
  *p = 20;
}

void testAssertCheck() {
  int* p = nullptr;
  assert(p != nullptr);
  *p = 10;
}
