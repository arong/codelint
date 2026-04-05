#include <iostream>

// False Positive Test - Returns reference but not of static local
class ReferenceParams {
public:
    static int& getIntRef(int& param) {
        return param; // Returns reference to PARAMETER, not static local
    }
};

class RefFromExternal {
public:
    static int& getRef(int& externVar) {
        return externVar; // Points to externally provided var
    }
};

int globalVar = 99;

int main() {
    int localVar = 55;
    int& ref1 = ReferenceParams::getIntRef(localVar); // Gets reference to parameter
    int& ref2 = RefFromExternal::getRef(globalVar);
    std::cout << "Refs: " << ref1 << ", " << ref2 << std::endl;
    return 0;
}