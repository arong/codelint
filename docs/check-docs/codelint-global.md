# codelint-global

Detects global variables in C++ code.

## Description

This check identifies variables declared at file scope (global variables) and reports them for review. Global variables can lead to hidden dependencies and make code harder to reason about.

## Examples

### Example 1: Simple global variable

```cpp
// Detected
int global_counter = 0;
const char* app_name = "MyApp";
```

### Example 2: Multiple globals

```cpp
// All detected
namespace {
  int config_value = 42;  // Global in anonymous namespace
}

double threshold = 0.5;   // Global at file scope
```

### Example 3: Not detected - local variables

```cpp
// NOT reported - these are local
void f() {
  int local_var = 10;
  static int static_local = 20;
}
```

## Detection Rules

**Detected:**
- Variables at file/namespace scope
- Const global variables
- Global variables in anonymous namespaces

**Not Detected:**
- Local variables (function scope)
- Static local variables (function scope)
- Function parameters
- Class/struct member variables
- Extern declarations (without definition)

## Rationale

Global variables introduce hidden state and dependencies that can make code:
- Harder to test
- More difficult to reason about
- Prone to thread-safety issues
- Difficult to refactor

Consider using:
- Dependency injection
- Singleton pattern (when appropriate)
- Function-local static variables

## See Also

- [cppcoreguidelines-avoid-non-const-global-variables](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/avoid-non-const-global-variables.html)
