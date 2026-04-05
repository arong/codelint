// Test file for array modification detection
// Test cases to verify array element modification detection

// Case 1: Array with element modification - should NOT suggest const
int arr1[10];
void test_modify_element() {
    arr1[5] = 42;
}

// Case 2: Empty initialized array, never modified - SHOULD suggest const
int arr2[10] = {};

// Case 3: Array with initializer list, never modified - SHOULD suggest const
int arr3[5] = {1, 2, 3, 4, 5};

int main() {
    return 0;
}
