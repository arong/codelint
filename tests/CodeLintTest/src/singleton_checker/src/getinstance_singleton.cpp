// Test 2: getInstance Naming Convention Singleton
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager manager;
        return manager;
    }
};
