// RUN: %codelint %s codelint-global-const-string %t
#include <string>

const std::string path = "/usr/local";
// CHECK-MESSAGES: :4:19: warning: global const std::string initialized with string literal should be 'constexpr const char*'  [codelint-global-const-string]
const std::string msg{"hello world"};
// CHECK-MESSAGES: :6:19: warning: global const std::string initialized with string literal should be 'constexpr const char*'  [codelint-global-const-string]
const std::string empty{""};
// CHECK-MESSAGES: :8:19: warning: global const std::string initialized with string literal should be 'constexpr const char*'  [codelint-global-const-string]
const auto name = std::string("codelint");
// CHECK-MESSAGES: :10:12: warning: global const std::string initialized with string literal should be 'constexpr const char*'  [codelint-global-const-string]

// === Expected Fixed Output ===
// CHECK-FIXES: #include <string>
// CHECK-FIXES: constexpr const char* path{"/usr/local"};
// CHECK-FIXES: constexpr const char* msg{"hello world"};
// CHECK-FIXES: constexpr const char* empty{""};
// CHECK-FIXES: constexpr const char* name{"codelint"};
