# codelint-init

Checks for proper variable initialization style in C++ code.

## Description

This check enforces modern C++ initialization best practices:

1. **Uninitialized variables** - Variables must be explicitly initialized
2. **Brace initialization** - Prefer `{}` over `=` syntax (except for `auto`)
3. **Auto type handling** - Auto types should use `=` assignment, not `{}`
4. **Unsigned suffix** - Add `U` suffix to unsigned integer literals
5. **Macro skip** - Automatically skips variables defined inside macros
6. **C-style arrays** - Provides specific warning for uninitialized arrays
7. **Equals-brace syntax** - Suggests `{}` instead of `= {}` syntax for non-auto types

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

### Example 2: Equals to brace initialization (non-auto types)

```cpp
// Before
int value = 42;

// After
int value{42};
```

### Example 3: Auto type - brace to equals conversion

Auto types use type deduction and should use `=` assignment syntax:

```cpp
// Before
auto x{42};
auto* p{&value};
const auto* cp{&value};

// After
auto x = 42;
auto* p = &value;
const auto* cp = &value;
```

### Example 4: Unsigned suffix

```cpp
// Before
unsigned int count = 5;

// After
unsigned int count = 5U;
```

### Example 5: C-style array initialization

```cpp
// Before
int arr[5];

// After
int arr[5]{};
```

### Example 6: Equals-brace to pure brace (non-auto types)

```cpp
// Before
int arr[5] = {};
int values[3] = {1, 2, 3};

// After
int arr[5]{};
int values[3]{1, 2, 3};
```

### Example 7: Auto types with equals (correct - no warning)

```cpp
// These will NOT trigger warnings - correct auto usage
auto x = 42;
auto* p = &value;
const auto* cp = &value;
auto& ref = value;
```

### Example 8: Macro definitions (skipped)

```cpp
// These will NOT trigger warnings
#define MACRO_VAR int x
MACRO_VAR;

#define DECLARE(type, name) type name
DECLARE(int, my_var);
```

## Auto Type Handling

Auto types require special handling because they use type deduction:

| Syntax | Auto Type | Non-Auto Type |
|--------|-----------|---------------|
| `auto x = 42` | ✅ Correct | - |
| `auto x{42}` | ❌ Warn → `auto x = 42` | - |
| `int x = 42` | - | ❌ Warn → `int x{42}` |
| `int x{42}` | - | ✅ Correct |

**Why auto types use `=` instead of `{}`:**

1. `auto x{42}` deduces to `std::initializer_list<int>` in some contexts
2. `auto x = 42` is the conventional and clearer syntax for type deduction
3. Brace initialization with auto can lead to surprising type deductions

## Skipped Cases

The check intentionally skips:

- **For loop variables** - `for (int i = 0; ...)` pattern
- **Union members** - Union initialization has special semantics
- **Extern declarations** - `extern int x;` is a declaration, not definition
- **Exception variables** - `catch (const auto& e)` pattern
- **Macro definitions** - Variables inside `#define` macros are not modified
- **Auto references** - `auto& ref = x;` uses `=` correctly
- **Multiple initializer list values** - `auto x = {1, 2, 3}` (no single-value conversion)

## Limitations

- Does not detect const/constexpr opportunities (requires CFG analysis)
- Does not perform flow-sensitive modification tracking

## See Also

- [cppcoreguidelines-pro-type-member-init](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/pro-type-member-init.html)
- [modernize-use-auto](https://clang.llvm.org/extra/clang-tidy/checks/modernize/use-auto.html)
