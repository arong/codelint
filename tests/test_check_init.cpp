#include <iostream>
#include <string>
#include <vector>

void test_initialization() {
  // Test 1: Uninitialized variables (should report)
  int a;
  char b;
  double c;
  unsigned int d;
  bool e;

  // Test 2: Variables initialized with '=' (should report use_equals_init)
  int f = 10;
  unsigned int g = 42;
  double h = 3.14;
  char i = 'x';
  bool j = true;

  // Test 3: Unsigned without suffix (should report unsigned_without_suffix)
  unsigned int k = 100;
  unsigned long l = 2000;
  unsigned int m = 5;

  // Test 4: Proper brace initialization (should NOT report)
  int n{10};
  unsigned int o{42U};
  double p{3.14};
  bool q{true};

  // Test 5: Unsigned with proper suffix (should NOT report)
  unsigned int r = 100U;
  unsigned long s = 2000UL;
  unsigned int t = 5U;

  // Test 6: Brace init with unsigned suffix (best practice)
  unsigned int u{100U};
  unsigned long v{2000UL};

  // Test 7: Multiple issues on same variable
  unsigned int w = 42;  // Both use_equals_init and unsigned_without_suffix

  // Test 8: Local scope
  {
    int x = 5;
    unsigned int y = 10;
  }

  // Test 9: Non-builtin types (should report)
  std::string str = "hello";
  std::vector<int> vec = {1, 2, 3};
}

class TestClass {
 public:
  void memberFunction() {
    int a = 5;            // Should report
    unsigned int b = 10;  // Should report twice

    int c{5};             // Should NOT report
    unsigned int d{10U};  // Should NOT report
  }
};

int main() {
  test_initialization();
  return 0;
}
