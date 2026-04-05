// Test 3: Singleton in Namespace
namespace Config {
    class Settings {
    public:
        static Settings& instance() {
            static Settings cfg;
            return cfg;
        }
    };
}
