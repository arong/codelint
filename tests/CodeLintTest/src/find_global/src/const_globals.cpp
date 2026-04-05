// Test file: const_globals.cpp
// Scenario: Const and constexpr globals
// Expected: 3 global variables detected

#include <string>

const int const_val = 100;
constexpr int constexpr_val = 200;
const std::string const_str = "const";

int main() {
    return 0;
}
