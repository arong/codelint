// RUN: %check_codelint %s codelint-singleton %t -- -std=c++17
// Test 1: Classic Meyer's Singleton Pattern
class Database {
public:
    static Database& instance() {
// CHECK-MESSAGES: :[@LINE]:22: warning: Meyer's Singleton pattern detected in 'instance'  [codelint-singleton]
        static Database inst;
        return inst;
    }
};
