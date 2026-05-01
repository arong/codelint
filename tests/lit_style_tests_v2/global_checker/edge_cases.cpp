// RUN: %codelint %s codelint-global %t

// Function-local static variables should NOT be detected
void counter() {
  static int call_count = 0;
  call_count++;
}

void helper() {
  static double cached_value = 3.14;
}

// Class/struct member variables should NOT be detected
class MyClass {
public:
  int public_member;

private:
  double private_member;
  static int static_member;
};

struct MyStruct {
  int field;
};

int main() {
  counter();
  helper();
  return 0;
}
