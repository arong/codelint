// Test: extern declarations should be SKIPPED
// Expected: Only 1 variable detected (defined_var), extern declarations skipped
#include <string>

extern int external_var;  // declaration only, should NOT be detected
extern std::string external_str;  // should NOT be detected
int defined_var = 42;  // definition, should be detected
