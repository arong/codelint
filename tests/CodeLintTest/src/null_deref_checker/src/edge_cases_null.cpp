// Test: Edge cases for null pointer analysis
// Expected warnings: codelint-null-deref (selective)

#include <cstddef>
#include <utility>

// Test 1: Null in union (WARNING - union members share storage)
union IntPointerUnion {
  int* ptr;
  int value;
};

void testUnionNull() {
  IntPointerUnion u;
  u.value = 10; // Sets union through value
  *u.ptr = 20;  // WARNING: ptr may not be valid
}

// Test 2: Volatile pointer (WARNING)
void testVolatilePointer() {
  volatile int* p = nullptr;
  *p = 10; // WARNING: dereferencing volatile null
}

// Test 3: Atomic pointer (WARNING)
#include <atomic>
void testAtomicPointer() {
  std::atomic<int*> p(nullptr);
  int* raw = p.load();
  *raw = 10; // WARNING: loaded value is null
}

// Test 4: Alignas pointer (WARNING)
struct alignas(64) AlignedData {
  int x;
};

void testAlignedPointer() {
  AlignedData* p = nullptr;
  p->x = 10; // ERROR: p is null
}

// Test 5: Packed struct pointer (WARNING)
#pragma pack(push, 1)
struct PackedData {
  char c;
  int* p;
};
#pragma pack(pop)

void testPackedStruct() {
  PackedData d;
  d.p = nullptr;
  *d.p = 10; // ERROR: d.p is null
}

// Test 6: Bitfield pointer - unusual (WARNING)
struct BitfieldStruct {
  int* ptr;
  int flags : 8;
};

void testBitfieldStruct() {
  BitfieldStruct s;
  s.ptr = nullptr;
  *s.ptr = 10; // ERROR: ptr is null
}

// Test 7: Anonymous struct pointer (WARNING)
struct WithAnonymous {
  struct {
    int* p;
  };
};

void testAnonymousStruct() {
  WithAnonymous w;
  w.p = nullptr;
  *w.p = 10; // ERROR: w.p is null
}

// Test 8: Flexible array member (FAM) - C99 extension (WARNING)
struct FlexibleArray {
  int len;
  int data[]; // FAM - size determined at runtime
};

void testFlexibleArray() {
  // This is a simplified test - actual FAM requires dynamic allocation
  int* p = nullptr;
  *p = 10; // ERROR: null dereference
}

// Test 9: Complex compound literal (WARNING)
void testCompoundLiteral() {
  int* p = (int[]){1, 2, 3}; // Array compound literal
  *p = 10;                   // OK: compound literal creates valid array
}

// Test 10: Designated initializer (WARNING)
struct DesignatedInit {
  int* a;
  int* b;
};

void testDesignatedInit() {
  DesignatedInit d = {.a = nullptr, .b = nullptr};
  *d.a = 10; // ERROR: d.a is null
}

// Test 11: Vector type (if supported) (WARNING)
#ifdef __GNUC__
typedef int __attribute__((vector_size(16))) IntVec;
void testVectorType() {
  int* p = nullptr;
  IntVec v = __builtin_ia32_loadups(reinterpret_cast<const float*>(p)); // Complex
}
#endif

// Test 12: Complex number pointer (WARNING)
#include <complex>
void testComplexPointer() {
  std::complex<double>* p = nullptr;
  *p = std::complex<double>(1.0, 2.0); // ERROR: null dereference
}

// Test 13: Enum pointer (WARNING)
enum Color { RED, GREEN, BLUE };

void testEnumPointer() {
  Color* p = nullptr;
  *p = RED; // ERROR: null dereference
}

// Test 14: Function pointer array (WARNING)
void funcA() {
}
void funcB() {
}

void testFunctionPointerArray() {
  void (*fp[2])() = {funcA, funcB};
  fp[0](); // OK: valid function pointer
  void (*nullFp[2])() = {nullptr, nullptr};
  nullFp[0](); // ERROR: null function pointer call
}

// Test 15: Pointer to void then cast (WARNING)
void testVoidPointerCast() {
  void* v = nullptr;
  int* p = static_cast<int*>(v);
  *p = 10; // ERROR: p is null
}

// Test 16: const_cast removing const (WARNING)
void testConstCast() {
  const int x = 10;
  const int* cp = &x;
  int* p = const_cast<int*>(cp);
  *p = 20; // OK: removing const, pointer is valid

  const int* cp2 = nullptr;
  int* p2 = const_cast<int*>(cp2);
  *p2 = 30; // ERROR: p2 is null
}

// Test 17: reinterpret_cast between pointer types (WARNING)
void testReinterpretCastTypes() {
  double d = 3.14;
  int* p = reinterpret_cast<int*>(&d);
  *p = 10; // OK: reinterpret of valid pointer

  int* p2 = reinterpret_cast<int*>(nullptr);
  *p2 = 20; // ERROR: p2 is null
}

// Test 18: Dynamic_cast upcast then downcast (WARNING)
void testCastChain(Base* base) {
  // Assuming Base is polymorphic from previous file
  // This is just a placeholder - actual test needs proper inheritance
  int* p = nullptr;
  *p = 10; // ERROR
}

// Test 19: Pointer arithmetic to valid then invalid (WARNING)
void testPointerArithmeticValidToInvalid() {
  int arr[10];
  int* p = arr;
  p = p + 5;  // Still valid (within array)
  *p = 10;    // OK: within bounds
  p = p + 10; // Now invalid (past end)
  *p = 20;    // WARNING: out of bounds (different check)
}

// Test 20: Null check with side effects (WARNING)
int globalCounter = 0;

int* getPointerWithSideEffect() {
  globalCounter++;
  return nullptr;
}

void testSideEffectInCheck() {
  int* p = getPointerWithSideEffect();
  if ((p = getPointerWithSideEffect()) != nullptr) {
    *p = 10; // OK: assigned and checked
  }
  // p may have been reassigned
}

// Forward declaration for test 18
class Base;
