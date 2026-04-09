// Test for multiple declarators on one line
// P0-1: All variables in multi-declarator statements should be checked

void test_multiple_declarators() {
  int a{}, b{}, c{};                // All three should trigger warnings
  double x{1.0}, y{}, z{3.0}; // Only y should trigger warning
  char *p1{}, *p2{}, *p3{};         // All three should trigger warnings

  int arr1[5]{}, arr2[10]{}; // Both should trigger C-style array warnings
  float f1{}, f2{}, f3{};  // Only f2 should trigger warning
}

void test_mixed_initialization() {
  int initialized{10}, uninitialized{}, also_initialized{20}; // Only uninitialized should warn
  unsigned u1{1U}, u2{}, u3{3U}; // u2 should warn, and suggest U suffix
}

void test_single_declarators() {
  int single{};        // Should trigger warning
  int initialized{}; // Should NOT trigger warning
}

class TestClass {
public:
  void method() {
    int m1{}, m2{}, m3{};     // All three should trigger warnings
    bool b1{true}, b2{}; // Only b2 should trigger warning
  }
};

int global1{}, global2{}, global3{}; // All three should trigger warnings

static int s1{}, s2{}; // Only s1 should trigger warning