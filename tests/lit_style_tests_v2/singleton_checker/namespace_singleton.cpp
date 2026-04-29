// RUN: %codelint %s codelint-singleton %t
// Test 3: Singleton in Namespace
namespace Config {
    class Settings {
    public:
        static Settings& instance() {
// CHECK-MESSAGES: :6:26: warning: Meyer's Singleton pattern detected in 'instance'  [codelint-singleton]
            static Settings cfg;
            return cfg;
        }
    };
}
