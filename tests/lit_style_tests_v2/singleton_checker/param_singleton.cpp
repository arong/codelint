// RUN: %check_codelint %s codelint-singleton %t -- -std=c++17
// Test file: param_singleton.cpp
// Scenario: getInstance with parameter - may not match classic pattern
// Expected: 0 or 1 singleton patterns (depends on matcher strictness)

#include <string>

class Config {
public:
  // This takes a parameter - still matches reference return + static local
  static Config& getInstance(const std::string& name) {
// CHECK-MESSAGES: :[@LINE]:18: warning: Meyer's Singleton pattern detected in 'getInstance'  [codelint-singleton]
    static Config cfg;
    return cfg;
  }

private:
  Config() {
  }
};

int main() {
  Config& cfg = Config::getInstance("default");
  return 0;
}
