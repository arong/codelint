// RUN: %check_codelint %s codelint-global-const-string %t
#include <string>

// Multi-declarator: warn but do not emit a fix-it that deletes siblings.
static const std::string kC = "s1", kD = "s2";
// CHECK-MESSAGES: warning: global const std::string initialized with string literal should be
// 'constexpr const char*'  [codelint-global-const-string]
// CHECK-MESSAGES: warning: global const std::string initialized with string literal should be
// 'constexpr const char*'  [codelint-global-const-string]

// Multi-arg ctor must not be rewritten (would change value from "abc" to "abcdef").
const std::string kB("abcdef", 3);

// CHECK-FIXES: static const std::string kC = "s1", kD = "s2";
// CHECK-FIXES: const std::string kB("abcdef", 3);
