// RUN: %check_codelint %s codelint-singleton %t
// Test 1: Classic Meyer's Singleton Pattern
class Database {
public:
  static Database& instance() {
    // CHECK-MESSAGES: [[@LINE-1]]:22: warning: Meyer's Singleton pattern detected in 'instance'
    // [codelint-singleton]
    static Database inst;
    return inst;
  }
};
