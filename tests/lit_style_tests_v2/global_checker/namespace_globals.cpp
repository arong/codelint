// RUN: %codelint %s codelint-global %t
// Test file: namespace_globals.cpp (MERGED: namespace_globals.cpp + anon_namespace_globals.cpp)
// Scenario: Global variables inside namespaces
// Expected: 4 global variables detected (2 named + 2 anonymous)

namespace MyApp {
int app_config = 100; // Namespace-level global - SHOULD detect
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'app_config' detected  [codelint-global]
const int kMaxSize = 50; // Const namespace global - SHOULD detect
// CHECK-MESSAGES: :[@LINE-1]:11: warning: global variable 'kMaxSize' detected  [codelint-global]
} // namespace MyApp

#include <string>

namespace {
int anon_var1 = 10; // Anonymous namespace var
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'anon_var1' detected  [codelint-global]
std::string anon_var2 = "test"; // Anonymous namespace string
// CHECK-MESSAGES: :[@LINE-1]:13: warning: global variable 'anon_var2' detected  [codelint-global]
} // namespace

int main() {
  return 0;
}
