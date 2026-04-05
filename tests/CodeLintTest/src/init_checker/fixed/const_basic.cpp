// Test file for basic const detection
// Tests const eligibility detection in various scenarios

void test_const_basic() {
    // 1. const-eligible: variable initialized once, never modified
    // Should suggest adding 'const' in the lint tool
    const int x{5};

    // 2. Modified: variable is modified after initialization
    // Should NOT suggest const
    int y{5};
    y = 10;

    // 3. Already const: already has const qualifier
    // Should be skipped (already const)
    const int z{5};

    // 4. Reference: reference types are out of scope for const detection
    // Should be skipped
    int& r = y;

    // Additional test cases for comprehensive coverage
    // 5. Pointer to const - should be skipped (pointer handling)
    const int* ptr_const{&y};

    // 6. Const pointer - should be skipped (pointer handling)
    int* const ptr_to_const{&y};

    // 7. Multiple const-eligible variables
    const int a{10};
    const int b{20};
    const int c{30};

    // 8. Modified after initialization (multiple modifications)
    int d{100};
    d = 200;
    d = 300;
}

int main() {
    test_const_basic();
    return 0;
}