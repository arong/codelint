// Test: Advanced null pointer patterns
// Expected warnings: codelint-null-deref

#include <cassert>
#include <cstddef>

// Test 1: Null after dynamic_cast failure (WARNING)
class Base {
public:
  virtual ~Base() = default;
};

class Derived : public Base {
public:
  void derivedOnly() {
  }
};

void testDynamicCastNull(Base* base) {
  Derived* d = dynamic_cast<Derived*>(base);
  d->derivedOnly(); // WARNING: d may be null if cast fails
}

// Test 2: Safe dynamic_cast with check (OK)
void testDynamicCastSafe(Base* base) {
  if (Derived* d = dynamic_cast<Derived*>(base)) {
    d->derivedOnly(); // OK: checked in if condition
  }
}

// Test 3: Null from failed allocation (WARNING)
void testFailedAllocation() {
  // This is a simplified test - in real code malloc may return null
  int* p = static_cast<int*>(nullptr); // Simulating failed malloc
  *p = 10;                             // ERROR: p is null
}

// Test 4: Null from failed new (WARNING - though new typically throws)
void testFailedNew() {
  // new typically throws bad_alloc, but with nothrow:
  int* p = new (std::nothrow) int(10);
  *p = 20; // WARNING: p may be null if allocation failed
  delete p;
}

// Test 5: Null from reinterpret_cast of 0 (ERROR)
void testReinterpretCastNull() {
  int* p = reinterpret_cast<int*>(0);
  *p = 10; // ERROR: p is null
}

// Test 6: Null from C-style cast of 0 (ERROR)
void testCStyleCastNull() {
  int* p = (int*)0;
  *p = 10; // ERROR: p is null
}

// Test 7: Null from static_cast of nullptr (ERROR)
void testStaticCastNull() {
  void* v = nullptr;
  int* p = static_cast<int*>(v);
  *p = 10; // ERROR: p is null
}

// Test 8: Pointer arithmetic on null (WARNING)
void testPointerArithmeticNull() {
  int* p = nullptr;
  p = p + 5; // Undefined but may not crash
  *p = 10;   // ERROR: dereferencing offset from null
}

// Test 9: Array decay to pointer then null dereference (ERROR)
void testArrayDecayNull() {
  int* p = nullptr;
  int (&ref)[5] = *reinterpret_cast<int (*)[5]>(p); // Complex: dereferencing null
}

// Test 10: Function pointer call through null (ERROR)
void testFunctionPointerNull() {
  void (*fp)(void) = nullptr;
  fp(); // ERROR: calling null function pointer
}

// Test 11: Member function pointer call through null (ERROR)
class Callable {
public:
  void call() {
  }
};

void testMemberFunctionPointerNull() {
  void (Callable::*mfp)() = nullptr;
  Callable c;
  (c.*mfp)(); // ERROR: calling null member function pointer
}

// Test 12: Reference binding to null pointer (ERROR/UB)
void testReferenceBindingNull() {
  int* p = nullptr;
  int& ref = *p; // ERROR: dereferencing null to bind reference
  ref = 10;
}

// Test 13: Lambda capture of null pointer (WARNING)
void testLambdaCaptureNull() {
  int* p = nullptr;
  auto lambda = [p]() {
    *p = 10; // WARNING: captured p may be null
  };
  lambda();
}

// Test 14: Lambda capture by reference (WARNING)
void testLambdaCaptureRef() {
  int* p = nullptr;
  auto lambda = [&p]() {
    if (p != nullptr) {
      *p = 10; // OK: checked
    }
  };
  p = new int(20);
  lambda();
  delete p;
}

// Test 15: Structured binding with null (C++17) (ERROR)
struct Pair {
  int x, y;
};

void testStructuredBindingNull() {
  Pair* p = nullptr;
  auto [x, y] = *p; // ERROR: dereferencing null
}

// Test 16: Ranged-for with null (ERROR)
void testRangedForNull() {
  int* arr = nullptr;
  for (int x : {1, 2, 3}) { // OK: this uses literal array
    // not using arr
  }
}

// Test 17: Template with null pointer (WARNING)
template <typename T> void templateDeref(T* p) {
  *p = T{}; // WARNING: p may be null depending on instantiation
}

void instantiateTemplate() {
  int* p = nullptr;
  templateDeref(p); // WARNING at instantiation
}

// Test 18: Variadic template with null (WARNING)
template <typename... Args> void variadicDeref(Args... args) {
  // Implementation would deref first arg if it's a pointer
}

// Test 19: Null in constexpr context (compile-time error usually)
constexpr int constexprDeref(int* p) {
  return *p; // Would be compile error if p is null in constexpr context
}

// Test 20: Null in noexcept function (WARNING)
void noexceptDeref(int* p) noexcept {
  *p = 10; // WARNING: if p is null, this terminates
}
