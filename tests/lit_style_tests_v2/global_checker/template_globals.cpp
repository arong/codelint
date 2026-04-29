// RUN: %codelint %s codelint-global %t
// Test file: template_globals.cpp
// Scenario: Template variables
// Expected: template variable instances detected

template<typename T>
T template_var = T{};
// CHECK-MESSAGES: :7:3: warning: global variable 'template_var' detected  [codelint-global]

template<> int template_var<int> = 42;
// CHECK-MESSAGES: :10:16: warning: global variable 'template_var' detected  [codelint-global]

double d = template_var<double>;
// CHECK-MESSAGES: :13:8: warning: global variable 'd' detected  [codelint-global]

int main() {
    return 0;
}
