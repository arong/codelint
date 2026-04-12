// Test file: namespace_globals.cpp
// Scenario: Global variables inside named namespace
// Expected: 2 global variables detected

namespace MyApp {
int app_config = 100;    // Namespace-level global - SHOULD detect
const int kMaxSize = 50; // Const namespace global - SHOULD detect
} // namespace MyApp

int main() {
  return 0;
}
