// RUN: %codelint %s codelint-global %t
// Test file: thread_local_globals.cpp
// Scenario: Thread-local global variables
// Expected: 1 thread_local variable detected

thread_local double tl_value = 3.14;  // Thread-local double
// CHECK-MESSAGES: :6:21: warning: global variable 'tl_value' detected  [codelint-global]

int main() {
    return 0;
}
