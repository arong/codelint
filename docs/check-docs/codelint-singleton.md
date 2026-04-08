# codelint-singleton

Detects Meyer's Singleton pattern in C++ code.

## Description

This check identifies functions that implement the Meyer's Singleton pattern - a function that returns a reference to a static local variable.

## Examples

### Example 1: Classic Meyer's Singleton

```cpp
// Detected
class Singleton {
public:
  static Singleton& instance() {
    static Singleton inst;
    return inst;
  }
};
```

### Example 2: With initialization

```cpp
// Detected
class Config {
public:
  static Config& get() {
    static Config instance{"default.cfg"};
    return instance;
  }
private:
  Config(std::string_view filename);
};
```

### Example 3: Not detected - regular function

```cpp
// NOT a singleton pattern
int getCounter() {
  static int counter = 0;  // Returns int, not reference
  return ++counter;
}

// NOT a singleton pattern
int* getPtr() {
  static int value = 42;
  return &value;  // Returns pointer, not reference
}
```

## Detection Pattern

A function is flagged if it:
1. Returns a reference type (`T&` or `const T&`)
2. Contains a static local variable
3. Returns a reference to that static local variable

## Rationale

Meyer's Singleton is a legitimate pattern but has trade-offs:
- Thread-safe initialization (C++11+)
- Simple implementation
- No explicit destruction control

Potential concerns:
- Hidden dependencies
- Global state
- Testing difficulties
- Lifetime management in dynamic libraries

Consider documenting:
- Why singleton is appropriate
- Thread-safety requirements
- Initialization dependencies

## See Also

- [cppcoreguidelines-avoid-non-const-global-variables](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/avoid-non-const-global-variables.html)