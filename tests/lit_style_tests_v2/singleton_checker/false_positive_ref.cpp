// RUN: %check_codelint %s codelint-singleton %t -- -std=c++17
// Test 7: False Positive - Parameter Reference (NOT a singleton)
int& getRef(int& x) {
    return x;  // Returns parameter reference, not static local
}
