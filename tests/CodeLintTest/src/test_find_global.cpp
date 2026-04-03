#include <iostream>
#include <string>
#include <vector>

// Test 1: Basic global variables
int global_int = 42;
const int global_const = 100;
unsigned int global_uint = 5000;

// Test 2: Static and thread_local
static std::string global_static = "hello";
thread_local double global_thread_local = 3.14;

// Test 3: With different types
float global_float = 2.5f;
double global_double = 1.234;
char global_char = 'a';
bool global_bool = true;

// Test 4: Unsigned with suffix (should not report issue)
unsigned int global_uint_suffix = 42U;
unsigned long global_ulong_suffix = 100UL;

// Test 5: Class types
std::vector<int> global_vector;
std::string global_string = "test";

void local_function() {
  int local_var = 10; // Should not be detected
}

int main() {
  std::cout << "Testing global variables" << std::endl;
  return 0;
}
