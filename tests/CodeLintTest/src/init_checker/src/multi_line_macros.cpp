// Test for multi-line macro handling

#include <vector>

#define LOG_IF_CHANGED(var, expr)                                                                  \
  do {                                                                                             \
    auto oldValue = (var);                                                                         \
    auto newValue = (expr);                                                                        \
    bool needChange = DoSomething(newValue);                                                       \
    if (needChange) {                                                                              \
      var = newValue;                                                                              \
    }                                                                                              \
  } while (0)

#define INIT_CONTAINER(name, type, ...)                                                            \
  type name;                                                                                       \
  name = {__VA_ARGS__}

#define DECLARE_VARS                                                                               \
  int x;                                                                                           \
  int y;                                                                                           \
  int z

bool DoSomething(int);

void test_multi_line_macros() {
  int val = 0;
  LOG_IF_CHANGED(val, val + 1);
  INIT_CONTAINER(vec, std::vector<int>, 1, 2, 3);
}

void test_simple_macro_vars() {
  DECLARE_VARS;
}

void test_regular_vars() {
  int a;
  int b = 10;
}
