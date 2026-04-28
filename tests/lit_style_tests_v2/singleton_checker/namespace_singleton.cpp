// RUN: %check_codelint %s codelint-singleton %t -- -std=c++17
// Test 3: Singleton in Namespace
namespace Config {
    class Settings {
    public:
        static Settings& instance() {
// CHECK-MESSAGES: :[@LINE]:26: warning: Meyer's Singleton pattern detected in 'instance'  [codelint-singleton]
            static Settings cfg;
            return cfg;
        }
    };
}
