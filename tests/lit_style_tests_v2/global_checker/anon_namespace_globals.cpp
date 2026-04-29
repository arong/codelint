// RUN: %codelint %s codelint-global %t
// Test file: anon_namespace_globals.cpp
// Scenario: Anonymous namespace globals
// Expected: 2 global variables detected

#include <string>

namespace {
    int anon_var1 = 10;              // Anonymous namespace var
// CHECK-MESSAGES: :9:9: warning: global variable 'anon_var1' detected  [codelint-global]
    std::string anon_var2 = "test"; // Anonymous namespace string
// CHECK-MESSAGES: :11:17: warning: global variable 'anon_var2' detected  [codelint-global]
}

int main() {
    return 0;
}
