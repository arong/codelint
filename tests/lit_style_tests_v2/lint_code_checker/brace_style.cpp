// RUN: %codelint %s codelint-lint-code %t

#include <cstddef>
#include <cstdint>
#include <string>

int a = 10;
// CHECK-MESSAGES: :[@LINE-1]:5: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
double b = 3.14;
// CHECK-MESSAGES: :[@LINE-1]:8: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
std::string s = "hello";
// CHECK-MESSAGES: :[@LINE-1]:13: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

int c = {1};
// CHECK-MESSAGES: :[@LINE-1]:5: warning: initializer should use '{}' syntax instead of '= {}'
// [codelint-lint-code]
int d = {};
// CHECK-MESSAGES: :[@LINE-1]:5: warning: initializer should use '{}' syntax instead of '= {}'
// [codelint-lint-code]

auto x{42};
// CHECK-MESSAGES: :[@LINE-1]:6: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]
auto* p{&a};
// CHECK-MESSAGES: :[@LINE-1]:7: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]
const auto* cp{&a};
// CHECK-MESSAGES: :[@LINE-1]:13: warning: auto type should use '=' assignment instead of brace
// initialization  [codelint-lint-code]

unsigned u = 100;
// CHECK-MESSAGES: :[@LINE-1]:10: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]
uint64_t big = 42;
// CHECK-MESSAGES: :[@LINE-1]:11: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

unsigned ul_bad = 100u;
// CHECK-MESSAGES: :[@LINE-1]:10: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code] CHECK-MESSAGES: :[@LINE-2]:17: warning: unsigned integer literal should use
// uppercase 'U'/'L' suffix  [codelint-lint-code]
uint64_t ul2_bad = 42ul;
// CHECK-MESSAGES: :[@LINE-1]:11: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code] CHECK-MESSAGES: :[@LINE-2]:18: warning: unsigned integer literal should use
// uppercase 'U'/'L' suffix  [codelint-lint-code]

std::string str("world");
// CHECK-MESSAGES: :[@LINE-1]:13: warning: variable should use '{}' syntax for initialization
// [codelint-lint-code]

int valid{10};
auto valid2 = 42;
unsigned valid3{100U};
