// Test file: thread_local_globals.cpp
// Scenario: Thread-local global variables
// Expected: 1 thread_local variable detected

thread_local double tl_value = 3.14;  // Thread-local double

int main() {
    return 0;
}
