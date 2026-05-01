// RUN: %codelint %s codelint-init %t
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
  // CHECK-MESSAGES: :38:7: error: variable is not initialized  [codelint-init]
  int b = 10;
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: #define LOG_IF_CHANGED(var, expr) \
// CHECK-FIXES:   do { \
// CHECK-FIXES:     auto oldValue = (var); \
// CHECK-FIXES:     auto newValue = (expr); \
// CHECK-FIXES:     bool needChange = DoSomething(newValue); \
// CHECK-FIXES:     if (needChange) { \
// CHECK-FIXES:       var = newValue; \
// CHECK-FIXES:     } \ CHECK-FIXES:   } while (0)
// CHECK-FIXES: #define INIT_CONTAINER(name, type, ...) \
// CHECK-FIXES:   type name; \ CHECK-FIXES:   name = {__VA_ARGS__}
// CHECK-FIXES: #define DECLARE_VARS \
// CHECK-FIXES:   int x; \
