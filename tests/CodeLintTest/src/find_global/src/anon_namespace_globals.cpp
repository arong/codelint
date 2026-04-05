// Test file: anon_namespace_globals.cpp
// Scenario: Anonymous namespace globals
// Expected: 2 global variables detected

#include <string>

namespace {
    int anon_var1 = 10;              // Anonymous namespace var
    std::string anon_var2 = "test"; // Anonymous namespace string
}

int main() {
    return 0;
}
