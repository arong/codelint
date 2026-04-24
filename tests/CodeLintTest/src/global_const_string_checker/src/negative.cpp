#include <cstdlib>
#include <string>

std::string mutable_str = "changeable";

const std::string runtime_str = std::to_string(42);
const std::string concatenated = std::string("hello") + " world";

constexpr const char* already_ok = "done";

void func() {
  const std::string local = "local";
}

extern const std::string extern_str;

const std::wstring wide = L"wide";
