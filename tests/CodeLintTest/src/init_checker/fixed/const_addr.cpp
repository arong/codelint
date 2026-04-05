// Test file for address-of detection - verify const suggestion is NOT made when address is taken
#include <cstdint>
#include <iostream>

// 1. Basic address-of scenarios - address taken, should NOT suggest const
// These variables have their address taken, so const should NOT be suggested
void test_basic_address_of() {
    int x{};
    int* p1 = &x;  // Address taken - x could be modified through p1

    int y{};
    int* p2 = &y;
    y = 5;  // Modified after address taken

    double d{};
    double* pd = &d;  // Address taken - d could be modified through pd

    char c{};
    char* pc = &c;  // Address taken
}

// 2. Global variables with address taken
int global_x{};
int* global_ptr = &global_x;  // Address taken - should NOT suggest const

// 3. Multiple addresses taken from same variable
void test_multiple_addresses() {
    int val{};
    int* p1 = &val;
    int* p2 = &val;
    // val has address taken multiple times - should NOT suggest const
}

// 4. Address passed to function
void takes_int_ptr(int* ptr) {}
void test_address_to_function() {
    int num{};
    takes_int_ptr(&num);  // Address passed - should NOT suggest const
}

// 5. Array elements with address taken
void test_array_elements() {
    int arr[5]{};
    int* p = &arr[2];  // arr[2] address taken - should NOT suggest const for arr
}

// 6. Struct member with address taken
struct Point {
    int x{};
    int y{};
};
void test_struct_member() {
    Point pt{};
    int* px = &pt.x;  // pt.x address taken - should NOT suggest const for pt.x
    int* py = &pt.y;  // pt.y address taken - should NOT suggest const for pt.y
}

// 7. Reference does NOT count as address taken (reference is not address-of operator)
void test_reference() {
    int val{10};
    int& ref = val;  // Reference, NOT address-of - const CAN be suggested if not modified
    // This should still suggest const if val is never modified
}

// 8. Mixed: some variables with address taken, some without
void test_mixed() {
    int with_addr{};
    int* p1 = &with_addr;  // Address taken - should NOT suggest const

    int without_addr{42};  // No address taken - const CAN be suggested
    // This one could suggest const if never modified
}

// 9. Pointer to pointer
void test_pointer_to_pointer() {
    int x{};
    int* p = &x;     // x address taken
    int** pp = &p;   // p address taken - p could be modified through pp
}

// 10. Address-of in conditional
void test_conditional_address() {
    int a{};
    int* ptr;
    if (true) {
        ptr = &a;  // Address taken conditionally - should NOT suggest const
    }
}

int main() {
    test_basic_address_of();
    test_multiple_addresses();
    test_address_to_function();
    test_array_elements();
    test_struct_member();
    test_reference();
    test_mixed();
    test_pointer_to_pointer();
    test_conditional_address();

    // Also test that properly initialized variables with address taken work
    int initialized_with_addr{10};
    int* ptr_init = &initialized_with_addr;  // Address taken - should NOT suggest const

    return 0;
}