// Test 4: False Positive - Static Local Variable (NOT a singleton)
void helper() {
    static int counter = 0;  // Not a singleton, return type is not a reference
    counter++;
}
