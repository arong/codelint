// Test file: param_singleton.cpp
// Scenario: getInstance with parameter - may not match classic pattern
// Expected: 0 or 1 singleton patterns (depends on matcher strictness)

#include <string>

class Config {
public:
  // This takes a parameter - still matches reference return + static local
  static Config& getInstance(const std::string& name) {
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
