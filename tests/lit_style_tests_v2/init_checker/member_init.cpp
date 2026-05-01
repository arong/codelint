// RUN: %codelint %s codelint-init %t
// Test for class member variable initialization
// P0-2: Members should be initialized in constructors or via in-class initializers

#include <string>

class UninitializedMembers {
  int x;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'x' is not initialized in constructor
  // [codelint-init]
  int y;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'y' is not initialized in constructor
  // [codelint-init]
  double d;
  // CHECK-MESSAGES: :[@LINE-1]:10: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:10: error: member variable 'd' is not initialized in constructor
  // [codelint-init]
  char* ptr;
  // CHECK-MESSAGES: :[@LINE-1]:9: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:9: error: member variable 'ptr' is not initialized in constructor
  // [codelint-init]

public:
  UninitializedMembers() {
  } // All members uninitialized - should warn
};

class PartiallyInitialized {
  int a;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  int b;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'b' is not initialized in constructor
  // [codelint-init]
  int c;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'c' is not initialized in constructor
  // [codelint-init]

public:
  PartiallyInitialized() : a(1) {
  } // b and c uninitialized - should warn
};

class FullyInitialized {
  int x;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  int y;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]

public:
  FullyInitialized() : x(0), y(0) {
  } // All initialized - should NOT warn
};

class InClassInitializers {
  int x = 0;
  int y = 0;
  double d = 0.0;

public:
  InClassInitializers() {
  } // All have in-class initializers - should NOT warn
};

class MixedInitialization {
  int a = 10;
  int b;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'b' is not initialized in constructor
  // [codelint-init]
  int c = 30;

public:
  MixedInitialization() {
  } // Only b uninitialized - should warn
};

class MultipleConstructors {
  int value;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'value' is not initialized in constructor
  // [codelint-init]
  std::string name;
  // CHECK-MESSAGES: :[@LINE-1]:15: warning: field is not explicitly initialized  [codelint-init]

public:
  MultipleConstructors() {
  } // Both uninitialized - should warn
  MultipleConstructors(int v) : value(v) {
  } // name uninitialized - should warn
  MultipleConstructors(int v, const std::string& n) : value(v), name(n) {
  } // All initialized - should NOT warn
};

struct StructMembers {
  int x;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  double y;
  // CHECK-MESSAGES: :[@LINE-1]:10: error: field is not initialized  [codelint-init]
  char c;
  // CHECK-MESSAGES: :[@LINE-1]:8: error: field is not initialized  [codelint-init]
};

class StaticMembers {
  static int static_var; // Static members should NOT trigger warnings
  // CHECK-MESSAGES: :[@LINE-1]:14: error: variable is not initialized  [codelint-init]
  int instance_var; // Should trigger warning if not initialized
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
};

class ConstMembers {
  const int const_val;
  // CHECK-MESSAGES: :[@LINE-1]:13: error: field is not initialized  [codelint-init]
  int regular_val;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'regular_val' is not initialized in
  // constructor  [codelint-init]

public:
  ConstMembers() : const_val(42) {
  } // regular_val uninitialized - should warn
};

class ReferenceMembers {
  int& ref;
  int value;
  // CHECK-MESSAGES: :[@LINE-1]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[@LINE-2]:7: error: member variable 'value' is not initialized in constructor
  // [codelint-init]

public:
  ReferenceMembers(int& r) : ref(r) {
  } // value uninitialized - should warn
};

// === Expected Fixed Output ===
// CHECK-FIXES: #include <string>
// CHECK-FIXES: class UninitializedMembers {
// CHECK-FIXES:   int x{};
// CHECK-FIXES:   int y{};
// CHECK-FIXES:   double d{};
// CHECK-FIXES:   char* ptr{};
// CHECK-FIXES: public:
// CHECK-FIXES:   UninitializedMembers() {
// CHECK-FIXES:   } // All members uninitialized - should warn
// CHECK-FIXES: };
// CHECK-FIXES: class PartiallyInitialized {
// CHECK-FIXES:   int a{};
// CHECK-FIXES:   int b{};
// CHECK-FIXES:   int c{};
