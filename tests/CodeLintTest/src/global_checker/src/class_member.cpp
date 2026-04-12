// Test file: class_member.cpp
// Scenario: Class member variables (NOT globals)
// Expected: 0 global variables detected (false positive test)

class MyClass {
public:
  int public_member; // Class member - NOT a global

private:
  double private_member;    // Private member - NOT a global
  static int static_member; // Static class member - NOT a global
};

struct MyStruct {
  int field; // Struct field - NOT a global
};

int main() {
  return 0;
}
