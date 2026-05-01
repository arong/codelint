// RUN: %codelint %s codelint-global %t

namespace MyApp {
int app_config = 100;
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'app_config' detected  [codelint-global]
const int kMaxSize = 50;
// CHECK-MESSAGES: :[@LINE-1]:11: warning: global variable 'kMaxSize' detected  [codelint-global]
} // namespace MyApp

#include <string>

namespace {
int anon_var1 = 10;
// CHECK-MESSAGES: :[@LINE-1]:5: warning: global variable 'anon_var1' detected  [codelint-global]
std::string anon_var2 = "test";
// CHECK-MESSAGES: :[@LINE-1]:13: warning: global variable 'anon_var2' detected  [codelint-global]
} // namespace

int main() {
  return 0;
}
