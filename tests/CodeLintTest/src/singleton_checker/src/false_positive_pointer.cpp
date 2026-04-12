// Test 6: False Positive - Return Pointer (NOT a singleton)
class Resource {
public:
    static Resource* get() {  // Returns pointer, not reference
        static Resource res;
        return &res;
    }
};
