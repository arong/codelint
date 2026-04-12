// Test file: no_return_static.cpp
// Scenario: Static local variable without returning (NOT a singleton)
// Expected: 0 singleton patterns detected (false positive test)

class Cache {
public:
  static void clear() {
    static int cleared_count = 0; // Static but not returned
    cleared_count++;
  }

  static int get_count() {
    static int access_count = 0;
    access_count++;
    return access_count; // Returns int, not reference
  }
};

int main() {
  Cache::clear();
  return 0;
}
