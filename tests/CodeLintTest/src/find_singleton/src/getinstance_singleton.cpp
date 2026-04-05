// Test: getInstance naming convention
// Expected: 1 singleton detected
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager manager;
        return manager;
    }
};