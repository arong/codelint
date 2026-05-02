// RUN: %check_codelint %s codelint-global %t
// Test file: template_globals.cpp
// Scenario: Template variables
// Expected: template variable instances detected

template <typename T> T template_var = T{};
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: global variable 'template_var' detected
// [codelint-global]

template <> int template_var<int> = 42;
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: global variable 'template_var' detected
// [codelint-global]

double d = template_var<double>;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: global variable 'd' detected  [codelint-global]

int main() {
  return 0;
}
