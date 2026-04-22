#include <cstddef>
#include <cstdint>
#include <string>

int a{10};
double b{3.14};
std::string s{"hello"};

int c{1};
int d{};

auto x = 42;
auto* p = &a;
const auto* cp = &a;

unsigned u{100U};
uint64_t big{42UL};

std::string str{"world"};

int valid{10};
auto valid2 = 42;
unsigned valid3{100U};
