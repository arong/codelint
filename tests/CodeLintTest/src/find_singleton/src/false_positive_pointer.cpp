#include <iostream>

// False Positive Test - Returns pointer, not reference
class PointerSingleton {
public:
    static PointerSingleton* getInstance() {
        static PointerSingleton instance; // Returns POINTER, not reference
        return &instance;
    }

private:
    PointerSingleton() = default;
};

class ManagerPointer {
public:
    static ManagerPointer* get() {
        static ManagerPointer mgr;
        return &mgr; // Returns pointer
    }
};

int main() {
    PointerSingleton* p = PointerSingleton::getInstance();
    ManagerPointer* mgr = ManagerPointer::get();
    std::cout << "Pointers: " << p << ", " << mgr << std::endl;
    return 0;
}