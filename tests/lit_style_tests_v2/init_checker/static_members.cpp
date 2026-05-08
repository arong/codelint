// RUN: %check_codelint %s codelint-init %t

// C++14 compatible: static data members (declarations, not definitions)
// Static data members are VarDecl with isStaticDataMember() and should be skipped.

class WithStaticMembers {
  static int count;
  static const int kMaxSize;
  static const int kVersion = 1;
  static const double kPi;
  int instance_var;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'instance_var' is not initialized in
  // constructor [codelint-init]

public:
  WithStaticMembers() {
  }
};

class WithMultipleStatic {
  static int x;
  static int y;
  static const int kA = 10;
  static const int kB;
  static const unsigned kCount = 5;
  double val;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: field is not initialized  [codelint-init]

public:
  WithMultipleStatic() : val{0.0} {
  }
};

class MixedStaticAndInstance {
  static int counter;
  static const char kName[];
  int a;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'a' is not initialized in
  // constructor [codelint-init]
  int b;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized  [codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'b' is not initialized in
  // constructor [codelint-init]

public:
  MixedStaticAndInstance() {
  }
};

// Definitions of static members (outside class) — these are also static data
// member declarations and should not trigger warnings.
int WithStaticMembers::count = 0;
int WithMultipleStatic::x;
int MixedStaticAndInstance::counter;

// === Expected Fixed Output ===
// CHECK-FIXES: class WithStaticMembers {
// CHECK-FIXES:   static int count;
// CHECK-FIXES:   static const int kMaxSize;
// CHECK-FIXES:   static const int kVersion = 1;
// CHECK-FIXES:   static const double kPi;
// CHECK-FIXES:   int instance_var{};
// CHECK-FIXES: public:
// CHECK-FIXES:   WithStaticMembers() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: class WithMultipleStatic {
// CHECK-FIXES:   static int x;
// CHECK-FIXES:   static int y;
// CHECK-FIXES:   static const int kA = 10;
// CHECK-FIXES:   static const int kB;
// CHECK-FIXES:   static const unsigned kCount = 5;
// CHECK-FIXES:   double val{};
// CHECK-FIXES: public:
// CHECK-FIXES:   WithMultipleStatic() : val{0.0} {
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: class MixedStaticAndInstance {
// CHECK-FIXES:   static int counter;
// CHECK-FIXES:   static const char kName[];
// CHECK-FIXES:   int a{};
// CHECK-FIXES:   int b{};
// CHECK-FIXES: public:
// CHECK-FIXES:   MixedStaticAndInstance() {
// CHECK-FIXES:   }
// CHECK-FIXES: };
