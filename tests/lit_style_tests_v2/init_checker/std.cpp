// RUN: %check_codelint %s codelint-init %t -- -std=c++17
#include <string>

static std::string str1;
// CHECK-MESSAGES: :[@LINE]:20: warning: variable is not explicitly initialized  [codelint-init]
static std::string str2{}; // this should not trigger a warning
static std::string str3{"str"};
static std::string str4{str3}; // OK

// === Expected Fixed Output ===
// CHECK-FIXES: #include <string>
// CHECK-FIXES: static std::string str1{};
// CHECK-FIXES: static std::string str2{}; // this should not trigger a warning
// CHECK-FIXES: static std::string str3{"str"};
// CHECK-FIXES: static std::string str4{str3}; // OK
