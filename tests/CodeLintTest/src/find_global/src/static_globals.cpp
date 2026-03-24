// Test file: static_globals.cpp
// Scenario: Static global variables
// Expected: 2 static globals detected

#include <string>

static int static_int = 10;                  // Static int global
static std::string static_str = "test";      // Static std::string global

int main() {
    return 0;
}
