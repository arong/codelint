// clean.cpp - Edge case fixture for testing empty results
// This file contains valid C++ code with NO issues for codelint to detect
// Expected: codelint should report 0 issues

#include <iostream>
#include <string>
#include <vector>

// Proper brace initialization (should NOT trigger issues)
int main() {
    // Valid brace initialization
    int a{42};
    int b{};
    unsigned int c{100U};
    const int d{10};
    constexpr int e{20};

    // Auto with equals (correct usage for type deduction)
    auto ptr = std::make_unique<int>(5);

    // Standard containers
    std::string str{"hello"};
    std::vector<int> vec{1, 2, 3};

    // Properly initialized global
    static constexpr int kGlobalConstant{100};

    std::cout << a << b << c << d << e << std::endl;
    return 0;
}
