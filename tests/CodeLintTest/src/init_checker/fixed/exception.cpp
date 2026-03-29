// Test file for exception handling - Bug #2 verification
// The catch variable should NOT be reported as uninitialized
// because it's initialized by the catch clause itself

#include <stdexcept>
#include <string>
#include <iostream>

// Test 1: Simple catch with exception_ptr
void test_catch_exception_ptr() {
    try {
        throw std::runtime_error("test error");
    } catch (const std::exception& e) {
        // e is initialized by the catch clause - should NOT be flagged
        std::cout << e.what() << std::endl;
    }
}

// Test 2: Catch by value - copy initialized
void test_catch_by_value() {
    try {
        throw std::runtime_error("error");
    } catch (std::runtime_error e) {
        // e is copy-initialized - should NOT be flagged
        std::cout << e.what() << std::endl;
    }
}

// Test 3: Catch with multiple exceptions
void test_multiple_catch() {
    try {
        throw std::out_of_range("out of range");
    } catch (const std::out_of_range& e) {
        // e initialized - should NOT be flagged
        std::cout << e.what() << std::endl;
    } catch (const std::exception& e) {
        // e initialized - should NOT be flagged
        std::cout << e.what() << std::endl;
    }
}

// Test 4: Nested try-catch
void test_nested_try() {
    try {
        try {
            throw std::runtime_error("inner");
        } catch (const std::runtime_error& inner_e) {
            // inner_e initialized - should NOT be flagged
            std::cout << inner_e.what() << std::endl;
        }
    } catch (const std::exception& outer_e) {
        // outer_e initialized - should NOT be flagged
        std::cout << outer_e.what() << std::endl;
    }
}

// Test 5: Re-throw
void test_rethrow() {
    try {
        throw std::runtime_error("original");
    } catch (const std::exception& e) {
        // e initialized - should NOT be flagged
        std::cout << e.what() << std::endl;
        throw;  // rethrow
    }
}

// Test 6: Catch variable used after assignment
void test_catch_used() {
    try {
        throw std::string("error message");
    } catch (const std::string& msg) {
        // msg is initialized - should NOT be flagged
        std::string copy = msg;  // copy the catch variable
        std::cout << copy << std::endl;
    }
}

int main() {
    test_catch_exception_ptr();
    test_catch_by_value();
    test_multiple_catch();
    test_nested_try();
    test_catch_used();
    return 0;
}
