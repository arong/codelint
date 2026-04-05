// Test: Classic Meyer's Singleton pattern
// Expected: 1 singleton detected
class Database {
public:
    static Database& instance() {
        static Database inst;
        return inst;
    }
};
