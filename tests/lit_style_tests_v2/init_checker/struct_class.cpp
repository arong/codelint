// RUN: %check_codelint %s codelint-init %t
// Test for struct and class instance initialization
// Focus: global instances (member_init.cpp covers member definitions)

#include <string>

// 1. SIMPLE STRUCT
struct CStruct {
  int x;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  double y;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: error: field is not initialized  [codelint-init]
};
CStruct cs1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]

// 2. STRUCT WITH METHODS
struct StructWithMethod {
  int value;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  void foo() {
  }
};
StructWithMethod swm1;
// CHECK-MESSAGES: :[[@LINE-1]]:18: error: variable is not initialized  [codelint-init]

// 3. CLASS INSTANCE
class TestClass {
public:
  int value;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  std::string name;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: field is not explicitly initialized  [codelint-init]
  TestClass() : value(0) {
  }
};
TestClass tc1;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: variable is not explicitly initialized  [codelint-init]

// 4. UNION
union UnionType {
  int i;
  double d;
  char c;
};
UnionType ut1;
// CHECK-MESSAGES: :[[@LINE-1]]:11: error: variable is not initialized  [codelint-init]

// 5. NESTED STRUCT
struct Outer {
  struct Inner {
    int x;
    // CHECK-MESSAGES: :[[@LINE-1]]:9: error: field is not initialized  [codelint-init]
    int y;
    // CHECK-MESSAGES: :[[@LINE-1]]:9: error: field is not initialized  [codelint-init]
  } inner;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: error: field is not initialized  [codelint-init]
  int outer_val;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
} outer1;
// CHECK-MESSAGES: :[[@LINE-1]]:3: error: variable is not initialized  [codelint-init]

// 6. STRUCT WITH UNION
struct StructWithUnion {
  int tag;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  union {
    int ival;
    double dval;
    char* sval;
  } u;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: error: field is not initialized  [codelint-init]
} swu1;
// CHECK-MESSAGES: :[[@LINE-1]]:3: error: variable is not initialized  [codelint-init]

// 7. INHERITANCE
class BaseClass {
public:
  virtual void foo() {
  }
  int base_val;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
};
class DerivedClass : public BaseClass {
public:
  void foo() override {
  }
  int derived_val;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
};
DerivedClass dc1;
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: variable is not explicitly initialized  [codelint-init]

// === Expected Fixed Output ===
// CHECK-FIXES: #include <string>
// CHECK-FIXES: struct CStruct {
// CHECK-FIXES:   int x{};
// CHECK-FIXES:   double y{};
// CHECK-FIXES: };
// CHECK-FIXES: CStruct cs1{};
// CHECK-FIXES: struct StructWithMethod {
// CHECK-FIXES:   int value{};
// CHECK-FIXES:   void foo() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: StructWithMethod swm1{};
