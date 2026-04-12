// Test file for type conversion scenarios
// Covers: float receiving int, double receiving int, narrowing conversions

void test_type_conversions() {
  float f1 = 5;
  float f2 = 10;
  float f3 = 0;

  double d1 = 100;
  double d2 = 200;

  int i1 = 3.14;
  int i2 = 2.71;

  float f4{5.0f};
  double d3{100.0};
  int i3{42};
  bool b4{true};

  long double ld1 = 42;
}
