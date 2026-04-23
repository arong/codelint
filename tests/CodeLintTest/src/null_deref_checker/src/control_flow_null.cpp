// Test: Null pointer detection through control flow
// Expected warnings: codelint-null-deref

#include <cstddef>

// Test 1: Null check in loop condition (WARNING/ERROR depending on analysis)
void testLoopWithNullCheck(int n) {
  int* p = nullptr;
  for (int i = 0; i < n && p != nullptr; ++i) {
    *p = i; // Complex: may or may not be null depending on loop entry
  }
}

// Test 2: While loop with null check (WARNING)
void testWhileLoopNull() {
  int* p = nullptr;
  while (p == nullptr) {
    // p is null here
    p = new int(10);
  }
  *p = 20; // OK: after loop, p is not null
  delete p;
}

// Test 3: Do-while with null (WARNING)
void testDoWhileNull() {
  int* p = nullptr;
  do {
    *p = 10; // WARNING: may be null on first iteration
    p = new int(20);
  } while (p == nullptr);
  delete p;
}

// Test 4: Switch statement with null (ERROR)
void testSwitchNull(int x) {
  int* p = nullptr;
  switch (x) {
  case 1:
    *p = 10; // ERROR: p is null
    break;
  case 2:
    p = new int(20);
    *p = 30; // OK: p is assigned
    break;
  default:
    *p = 40; // ERROR: p is null
  }
  if (p != nullptr)
    delete p;
}

// Test 5: Early return with null check (OK/ERROR)
void testEarlyReturn(int* p) {
  if (p == nullptr) {
    return;
  }
  *p = 10; // OK: if we get here, p is not null
}

// Test 6: Complex branch - null in one path (ERROR)
void testComplexBranch(bool a, bool b) {
  int* p = nullptr;
  if (a) {
    if (b) {
      p = new int(10);
    }
  }
  *p = 20; // WARNING: may be null (depends on a and b)
  if (p != nullptr)
    delete p;
}

// Test 7: Ternary operator with null (ERROR)
void testTernaryNull(bool cond) {
  int* p = cond ? nullptr : new int(10);
  *p = 20; // WARNING: may be null if cond is true
  if (p != nullptr)
    delete p;
}

// Test 8: Logical OR with null check (WARNING)
void testLogicalOrNull(bool flag) {
  int* p = nullptr;
  int* q = new int(10);
  if (p != nullptr || flag) {
    *p = 20; // WARNING: may still be null (short-circuit)
  }
  delete q;
}

// Test 9: Logical AND with null check (OK)
void testLogicalAndNull() {
  int* p = nullptr;
  int x = 5;
  if (p != nullptr && x > 0) {
    *p = 10; // OK: if we get here, p is not null
  }
}

// Test 10: Goto with null (ERROR)
void testGotoNull() {
  int* p = nullptr;
  goto skip;
  p = new int(10);
skip:
  *p = 20; // ERROR: p is still null (goto skips assignment)
}

// Test 11: Break with null (WARNING)
void testBreakNull() {
  int* p = nullptr;
  for (int i = 0; i < 10; ++i) {
    if (i == 5) {
      break;
    }
    p = new int(i);
  }
  *p = 20; // WARNING: may be null if loop never assigns
  if (p != nullptr)
    delete p;
}

// Test 12: Continue with null (ERROR)
void testContinueNull() {
  int* p = nullptr;
  for (int i = 0; i < 10; ++i) {
    if (i < 5) {
      continue; // skips assignment for first 5 iterations
    }
    p = new int(i);
  }
  *p = 20; // WARNING: may be null if all iterations continue
  if (p != nullptr)
    delete p;
}
