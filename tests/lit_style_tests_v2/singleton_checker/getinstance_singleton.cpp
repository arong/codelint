// RUN: %codelint %s codelint-singleton %t
// Test 2: getInstance Naming Convention Singleton
class LogManager {
public:
    static LogManager& getInstance() {
// CHECK-MESSAGES: :5:24: warning: Meyer's Singleton pattern detected in 'getInstance'  [codelint-singleton]
        static LogManager manager;
        return manager;
    }
};
