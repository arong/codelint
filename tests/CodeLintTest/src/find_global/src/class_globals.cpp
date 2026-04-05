// Test: Global class/struct instances
// Expected: 2 global instances detected
struct MyStruct { int x; };
class MyClass { public: int value; };
MyStruct global_struct;
MyClass global_class;

