// Test file: typed_globals.cpp
// Scenario: Various type global variables
// Expected: 6 global variables detected

#include <vector>
#include <string>

float global_float = 3.14f;       // float
double global_double = 2.718;     // double
char global_char = 'A';           // char
bool global_bool = true;          // bool
std::vector<int> global_vec;      // std::vector
std::string global_str = "hello"; // std::string

int main() {
    return 0;
}
