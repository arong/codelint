// RUN: %codelint %s codelint-init %t
// Test file for type conversion scenarios
// Covers: float receiving int, double receiving int, narrowing conversions

void test_type_conversions() {
  float f1 = 5;
// CHECK-MESSAGES: :6:9: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
  float f2 = 10;
// CHECK-MESSAGES: :8:9: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
  float f3 = 0;
// CHECK-MESSAGES: :10:9: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

  double d1 = 100;
// CHECK-MESSAGES: :13:10: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
  double d2 = 200;
// CHECK-MESSAGES: :15:10: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

  int i1 = 3.14;
// CHECK-MESSAGES: :18:7: warning: narrowing conversion from floating to integer; cannot use '{}' initialization  [codelint-init]
  int i2 = 2.71;
// CHECK-MESSAGES: :20:7: warning: narrowing conversion from floating to integer; cannot use '{}' initialization  [codelint-init]

  float f4{5.0f};
  double d3{100.0};
  int i3{42};
  bool b4{true};

  long double ld1 = 42;
// CHECK-MESSAGES: :28:15: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void test_type_conversions() {
// CHECK-FIXES:   float f1{5};
// CHECK-FIXES:   float f2{10};
// CHECK-FIXES:   float f3{0};
// CHECK-FIXES:   double d1{100};
// CHECK-FIXES:   double d2{200};
// CHECK-FIXES:   int i1 = 3.14;
// CHECK-FIXES:   int i2 = 2.71;
// CHECK-FIXES:   float f4{5.0f};
// CHECK-FIXES:   double d3{100.0};
// CHECK-FIXES:   int i3{42};
// CHECK-FIXES:   bool b4{true};
// CHECK-FIXES:   long double ld1{42};
