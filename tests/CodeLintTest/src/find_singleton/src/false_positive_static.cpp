#include <iostream>

// False Positive Test - Static returns value type (not reference)
class NotSingleton {
public:
    static int getValue() {
        static int value = 100; // This is a static local but NOT a singleton (returns value)
        return value;
    }
};

// Another false positive
class Helper {
public:
    static int compute() {
        static int computed = 42;
        return computed;
    }
};

int main() {
    int val1 = NotSingleton::getValue();
    int val2 = Helper::compute();
    std::cout << "Values: " << val1 << ", " << val2 << std::endl;
    return 0;
}