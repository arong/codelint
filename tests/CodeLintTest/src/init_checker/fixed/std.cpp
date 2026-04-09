#include <string>

static std::string str1{};
static std::string str2{}; // this should not trigger a warning
static std::string str3{"str"};
static std::string str4{str3}; // OK