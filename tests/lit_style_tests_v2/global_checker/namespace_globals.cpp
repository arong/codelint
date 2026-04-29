// RUN: %codelint %s codelint-global %t
// Test file: namespace_globals.cpp
// Scenario: Global variables inside named namespace
// Expected: 2 global variables detected

namespace MyApp {
int app_config = 100;    // Namespace-level global - SHOULD detect
// CHECK-MESSAGES: :7:5: warning: global variable 'app_config' detected  [codelint-global]
const int kMaxSize = 50; // Const namespace global - SHOULD detect
// CHECK-MESSAGES: :9:11: warning: global variable 'kMaxSize' detected  [codelint-global]
} // namespace MyApp

int main() {
  return 0;
}
