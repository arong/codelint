#include <initializer_list>
#include <iostream>
#include <vector>

class MyArray {
   private:
   public:
    MyArray(std::initializer_list<int> list) {
        std::cout << "construct from initializer_list\n";
    }

    MyArray(int value) { std::cout << "construct from single value\n"; }
};

int test() {
    MyArray arr = 1; // shall not be changed to {}
    MyArray arr2{10};
}
