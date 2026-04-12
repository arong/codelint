// Test file: multi_singleton.cpp
// Scenario: Multiple Singleton classes in one file
// Expected: 2 singleton patterns detected

// Singleton 1: Database
class Database {
public:
  static Database& instance() {
    static Database inst;
    return inst;
  }

private:
  Database() {
  }
};

// Singleton 2: Logger
class Logger {
public:
  static Logger& getInstance() {
    static Logger log;
    return log;
  }

private:
  Logger() {
  }
};

int main() {
  Database& db = Database::instance();
  Logger& log = Logger::getInstance();
  return 0;
}
