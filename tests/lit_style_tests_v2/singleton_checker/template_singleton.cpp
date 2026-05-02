// RUN: %check_codelint %s codelint-singleton %t
// Test file: template_singleton.cpp
// Scenario: Template-based Singleton pattern
// Expected: 1 singleton pattern detected

template <typename T> class SingletonHolder {
public:
  static SingletonHolder& instance() {
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Meyer's Singleton pattern detected in 'instance'
    // [codelint-singleton]
    static SingletonHolder inst;
    return inst;
  }

private:
  SingletonHolder() {
  }
};

// Explicit instantiation
// Note: Template singleton detection may vary by implementation

int main() {
  return 0;
}
