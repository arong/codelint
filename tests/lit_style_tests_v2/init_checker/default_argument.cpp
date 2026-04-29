// RUN: %codelint %s codelint-init %t
#include <iostream>
void withArgument(int a, int b = 10) {
  std::cout << (a + b);
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <iostream>
// CHECK-FIXES: void withArgument(int a, int b = 10) {
// CHECK-FIXES:   std::cout << (a + b);
// CHECK-FIXES: }
