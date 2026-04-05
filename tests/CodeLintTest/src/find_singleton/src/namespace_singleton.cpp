// Test: Singleton inside namespace
// Expected: 1 singleton detected
namespace Config {
    class Settings {
    public:
        static Settings& instance() {
            static Settings cfg;
            return cfg;
        }
    };
}