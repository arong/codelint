// RUN: %check_codelint %s codelint-init %t

class Foo {
  static const int kMaxSize;
  static const int kVersion = 1;
  int instance_var;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: error: member variable 'instance_var' is not initialized in
  // constructor [codelint-init]

public:
  Foo() {
  }
};

class Bar {
  static const double kPi;
  int x;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: error: member variable 'x' is not initialized in constructor
  // [codelint-init]

public:
  Bar() {
  }
};

// === Expected Fixed Output ===
// CHECK-FIXES: class Foo {
// CHECK-FIXES:   static const int kMaxSize;
// CHECK-FIXES:   static const int kVersion = 1;
// CHECK-FIXES:   int instance_var{};
// CHECK-FIXES: public:
// CHECK-FIXES:   Foo() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: class Bar {
// CHECK-FIXES:   static const double kPi;
// CHECK-FIXES:   int x{};
// CHECK-FIXES: public:
// CHECK-FIXES:   Bar() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
