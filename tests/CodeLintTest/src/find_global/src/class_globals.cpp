// Test file: class_globals.cpp
// Scenario: Global class/struct instances
// Expected: 2 global variables detected

struct MyStruct { int x; };
class MyClass { public: int value; };

MyStruct global_struct;  // struct instance
MyClass global_class;    // class instance

int main() {
    return 0;
}
