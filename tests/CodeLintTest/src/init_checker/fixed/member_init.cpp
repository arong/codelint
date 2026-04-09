// Test for class member variable initialization
// P0-2: Members should be initialized in constructors or via in-class initializers

#include <string>

class UninitializedMembers {
  int x{};
  int y{};
  double d{};
  char* ptr{};

public:
  UninitializedMembers() {
  } // All members uninitialized - should warn
};

class PartiallyInitialized {
  int a{};
  int b{};
  int c{};

public:
  PartiallyInitialized() : a(1) {
  } // b and c uninitialized - should warn
};

class FullyInitialized {
  int x{};
  int y{};

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
  int b{};
  int c = 30;

public:
  MixedInitialization() {
  } // Only b uninitialized - should warn
};

class MultipleConstructors {
  int value{};
  std::string name{};

public:
  MultipleConstructors() {
  } // Both uninitialized - should warn
  MultipleConstructors(int v) : value(v) {
  } // name uninitialized - should warn
  MultipleConstructors(int v, const std::string& n) : value(v), name(n) {
  } // All initialized - should NOT warn
};

struct StructMembers {
  int x{};
  double y{};
  char c{};
};

class StaticMembers {
  static int static_var{}; // Static members should NOT trigger warnings
  int instance_var{};      // Should trigger warning if not initialized
};

class ConstMembers {
  const int const_val{};
  int regular_val{};

public:
  ConstMembers() : const_val(42) {
  } // regular_val uninitialized - should warn
};

class ReferenceMembers {
  int& ref;
  int value{};

public:
  ReferenceMembers(int& r) : ref(r) {
  } // value uninitialized - should warn
};