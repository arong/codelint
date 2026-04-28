// RUN: %check_codelint %s codelint-init %t -- -std=c++17
#include <initializer_list>
#include <iostream>
#include <vector>

class MyArray {
private:
public:
  MyArray(std::initializer_list<int> list) {
    std::cout << "construct from initializer_list\n";
  }

  MyArray(int value) {
    std::cout << "construct from single value\n";
  }
};

int test() {
  MyArray arr = 1; // shall not be changed to {}
  MyArray arr2{10};
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <initializer_list>
// CHECK-FIXES: #include <iostream>
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: class MyArray {
// CHECK-FIXES: private:
// CHECK-FIXES: public:
// CHECK-FIXES:   MyArray(std::initializer_list<int> list) {
// CHECK-FIXES:     std::cout << "construct from initializer_list\n";
// CHECK-FIXES:   }
// CHECK-FIXES:   MyArray(int value) {
// CHECK-FIXES:     std::cout << "construct from single value\n";
// CHECK-FIXES:   }
// CHECK-FIXES: };
// CHECK-FIXES: int test() {
// CHECK-FIXES:   MyArray arr = 1; // shall not be changed to {}
// CHECK-FIXES:   MyArray arr2{10};
// CHECK-FIXES: }
