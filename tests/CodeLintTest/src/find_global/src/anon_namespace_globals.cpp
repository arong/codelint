// Test: Variables in anonymous namespace
// Expected: 2 variables detected
#include <string>

namespace {
    int anon_var1 = 10;
    std::string anon_var2 = "test";
}

void test_function() {
    int local_var = 42; // Should NOT be detected
}
