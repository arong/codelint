// RUN: %codelint %s codelint-global %t
// Test file: static_globals.cpp
// Scenario: Static global variables
// Expected: 2 static globals detected

#include <string>

static int static_int = 10;                  // Static int global
// CHECK-MESSAGES: :8:12: warning: global variable 'static_int' detected  [codelint-global]
static std::string static_str = "test";      // Static std::string global
// CHECK-MESSAGES: :10:20: warning: global variable 'static_str' detected  [codelint-global]

int main() {
    return 0;
}
