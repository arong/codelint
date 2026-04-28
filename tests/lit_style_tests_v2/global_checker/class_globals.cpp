// RUN: %check_codelint %s codelint-global %t -- -std=c++17
// Test file: class_globals.cpp
// Scenario: Global class/struct instances
// Expected: 2 global variables detected

struct MyStruct { int x; };
class MyClass { public: int value; };

MyStruct global_struct;  // struct instance
// CHECK-MESSAGES: :[@LINE]:10: warning: global variable 'global_struct' detected  [codelint-global]
MyClass global_class;    // class instance
// CHECK-MESSAGES: :[@LINE]:9: warning: global variable 'global_class' detected  [codelint-global]

int main() {
    return 0;
}
