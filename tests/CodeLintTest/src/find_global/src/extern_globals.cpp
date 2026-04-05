// Test file: extern_globals.cpp
// Scenario: Extern declarations should be skipped
// Expected: 1 global variable detected (only defined_var)

#include <string>

extern int external_var;            // Declaration - should NOT be detected
extern std::string external_str;    // Declaration - should NOT be detected
int defined_var = 42;               // Definition - SHOULD be detected

int main() {
    return 0;
}
