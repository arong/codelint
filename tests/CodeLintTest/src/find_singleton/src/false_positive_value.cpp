// Test 5: False Positive - Return by Value (NOT a singleton)
class Factory {
public:
    static Factory create() {  // Returns by value, not by reference
        return Factory();
    }
};
