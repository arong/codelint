# codelint-init

Checks for proper variable initialization style in C++ code.

## Description

This check enforces modern C++ initialization best practices:

1. **Uninitialized variables** - Variables must be explicitly initialized
2. **Brace initialization** - Prefer `{}` over `=` syntax
3. **Unsigned suffix** - Add `U` suffix to unsigned integer literals
4. **Macro skip** - Automatically skips variables defined inside macros
5. **C-style arrays** - Provides specific warning for uninitialized arrays
6. **Equals-brace syntax** - Suggests `{}` instead of `= {}` syntax

## Examples

### Example 1: Uninitialized variable

```cpp
// Before
void f() {
  int x;
}

// After
void f() {
  int x{};
}
```

### Example 2: Equals to brace initialization

```cpp
// Before
int value = 42;

// After
int value{42};
```

### Example 3: Unsigned suffix

```cpp
// Before
unsigned int count = 5;

// After
unsigned int count = 5U;
```

### Example 4: C-style array initialization

```cpp
// Before
int arr[5];

// After
int arr[5]{};
```

### Example 5: Equals-brace to pure brace

```cpp
// Before
int arr[5] = {};
int values[3] = {1, 2, 3};

// After
int arr[5]{};
int values[3]{1, 2, 3};
```

### Example 6: Macro definitions (skipped)

```cpp
// These will NOT trigger warnings
#define MACRO_VAR int x
MACRO_VAR;

#define DECLARE(type, name) type name
DECLARE(int, my_var);
```

## Skipped Cases

The check intentionally skips:

- **Auto declarations** - `auto x = 5;` requires `=` syntax
- **For loop variables** - `for (int i = 0; ...)` pattern
- **Union members** - Union initialization has special semantics
- **Extern declarations** - `extern int x;` is a declaration, not definition
- **Exception variables** - `catch (const auto& e)` pattern
- **Macro definitions** - Variables inside `#define` macros are not modified

## Limitations

- Does not detect const/constexpr opportunities (requires CFG analysis)
- Does not perform flow-sensitive modification tracking

## See Also

- [cppcoreguidelines-pro-type-member-init](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/pro-type-member-init.html)
- [modernize-use-auto](https://clang.llvm.org/extra/clang-tidy/checks/modernize/use-auto.html)
