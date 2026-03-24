// Test for struct and class instance initialization
// Focus: global instances (member_init.cpp covers member definitions)

#include <string>

// 1. SIMPLE STRUCT
struct CStruct {
  int x;
  double y;
};
CStruct cs1;

// 2. STRUCT WITH METHODS
struct StructWithMethod {
  int value;
  void foo() {
  }
};
StructWithMethod swm1;

// 3. CLASS INSTANCE
class TestClass {
public:
  int value;
  std::string name;
  TestClass() : value(0) {
  }
};
TestClass tc1;

// 4. UNION
union UnionType {
  int i;
  double d;
  char c;
};
UnionType ut1;

// 5. NESTED STRUCT
struct Outer {
  struct Inner {
    int x;
    int y;
  } inner;
  int outer_val;
} outer1;

// 6. STRUCT WITH UNION
struct StructWithUnion {
  int tag;
  union {
    int ival;
    double dval;
    char* sval;
  } u;
} swu1;

// 7. INHERITANCE
class BaseClass {
public:
  virtual void foo() {
  }
  int base_val;
};
class DerivedClass : public BaseClass {
public:
  void foo() override {
  }
  int derived_val;
};
DerivedClass dc1;
