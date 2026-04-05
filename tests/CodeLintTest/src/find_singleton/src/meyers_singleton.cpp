// Test 1: Classic Meyer's Singleton Pattern
class Database {
public:
    static Database& instance() {
        static Database inst;
        return inst;
    }
};
