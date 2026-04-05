#include <iostream>

// False Positive Test - Returns value, not reference
class NotSingleton {
public:
    static NotSingleton create() {
        return NotSingleton(); // Returns value, not reference
    }

private:
    NotSingleton() = default;
};

class MathConstants {
public:
    static double PI() {
        static double pi = 3.14159;
        return pi; // Returns value, not reference
    }
    
    static int ZERO() {
        static int zero = 0; // Returns value
        return zero;
    }
};

int main() {
    NotSingleton obj = NotSingleton::create();
    double pi = MathConstants::PI();
    int zero = MathConstants::ZERO();
    std::cout << "Values: " << pi << ", " << zero << std::endl;
    return 0;
}