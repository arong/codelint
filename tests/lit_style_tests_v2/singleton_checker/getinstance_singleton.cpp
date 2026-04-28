// RUN: %check_codelint %s codelint-singleton %t -- -std=c++17
// Test 2: getInstance Naming Convention Singleton
class LogManager {
public:
    static LogManager& getInstance() {
// CHECK-MESSAGES: :[@LINE]:24: warning: Meyer's Singleton pattern detected in 'getInstance'  [codelint-singleton]
        static LogManager manager;
        return manager;
    }
};
