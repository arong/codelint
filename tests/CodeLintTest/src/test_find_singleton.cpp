#include <iostream>

// Test 1: Classic Meyer's singleton
class Singleton {
 public:
  static Singleton& instance() {
    static Singleton inst;
    return inst;
  }

  void doSomething() { std::cout << "Singleton doing something" << std::endl; }

 private:
  Singleton() {}
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
};

// Test 2: Singleton with getInstance() naming
class Manager {
 public:
  static Manager& getInstance() {
    static Manager mgr;
    return mgr;
  }

  void manage() { std::cout << "Manager managing" << std::endl; }

 private:
  Manager() {}
};

// Test 3: Singleton in a namespace
namespace MyApp {
class Config {
 public:
  static Config& instance() {
    static Config cfg;
    return cfg;
  }

 private:
  Config() {}
};
}  // namespace MyApp

// Test 4: Non-singleton static local variable (should NOT be detected)
class Helper {
 public:
  static int getValue() {
    static int value = 100;  // Not a singleton
    return value;
  }
};

// Test 5: Static method without local static (should NOT be detected)
class Calculator {
 public:
  static int add(int a, int b) { return a + b; }
};

// Test 6: Singleton returning value type (should NOT be detected)
class Factory {
 public:
  static Factory create() { return Factory(); }

 private:
  Factory() {}
};

int main() {
  // Using singletons
  Singleton& s = Singleton::instance();
  s.doSomething();

  Manager& m = Manager::getInstance();
  m.manage();

  MyApp::Config& c = MyApp::Config::instance();

  // These should not be detected as singletons
  int val = Helper::getValue();
  int sum = Calculator::add(5, 3);
  Factory f = Factory::create();

  return 0;
}