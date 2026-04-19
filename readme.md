# Codelint - clang-tidy Plugin for C++ Initialization Best Practices

Codelint is a **clang-tidy plugin** that enforces modern C++ initialization best practices, helping you write safer and more consistent code.

## Why Codelint?

### Problems Codelint Solves

| Problem | Risk | Codelint Solution |
|---------|------|-------------------|
| Uninitialized variables | Undefined behavior, crashes | Auto-fix with `{}` initialization |
| `int x = 5` style | Less explicit, allows narrowing | Auto-fix to `int x{5}` |
| `bool b = 1` | Dangerous implicit conversion | **Error warning** - force explicit |
| Missing `U`/`UL` suffix | Silent signed/unsigned bugs | Auto-fix with correct suffix |
| Uninitialized class members | Partial initialization bugs | Constructor initializer check |

### Real-World Example

```cpp
// Before: Dangerous code that Codelint catches
int Init() { return 0; }

void process() {
  int count;              // ❌ uninitialized - undefined behavior
  int value = 3.14;       // ❌ narrowing conversion
  unsigned u = 100;       // ❌ missing U suffix
  uint64_t big = 42;      // ❌ missing UL suffix
  bool ret = Init();      // ❌ ERROR: integer to bool is dangerous
}

// After: Safe code after Codelint auto-fix
void process() {
  int count{};            // ✅ explicitly initialized
  int value{3};           // ✅ explicit integer (user fixed narrowing)
  unsigned u{100U};       // ✅ proper U suffix for 32-bit
  uint64_t big{42UL};     // ✅ proper UL suffix for 64-bit
  bool ret{true};         // ✅ explicit bool (user fixed conversion)
}
```

## Checks

| Check | What It Does | Auto-fix | Language Support |
|-------|--------------|----------|------------------|
| **codelint-init** | Variable initialization style | ✅ Yes | C++14/17/20 |
| **codelint-strict-bool-condition** | Bool-only condition enforcement | ❌ No | C++14/17/20 |
| **codelint-global** | Global variable detection | ❌ No | C++ |
| **codelint-singleton** | Meyer's Singleton pattern detection | ❌ No | C++ |

## codelint-init Features

### 1. Uninitialized Variables → `{}`

Codelint distinguishes between **trivial** and **non-trivial** types:

| Type Category | Examples | Default Behavior | Diagnostic Level |
|---------------|----------|------------------|------------------|
| **Trivial** | `int`, `float`, `double`, pointers | Uninitialized = undefined behavior | **Error** |
| **Non-trivial** | `std::string`, `std::vector`, classes | Has default constructor | **Warning** |

```cpp
int x;                  // ❌ Error: trivial type uninitialized (undefined behavior)
std::string str;        // ⚠️ Warning: non-trivial type not explicitly initialized (well-defined default)
int arr[10];            // ❌ Error: trivial type array uninitialized
```

After auto-fix:

```cpp
int x{};                // ✅ fixed: trivial type initialized
std::string str{};      // ✅ fixed: non-trivial type explicitly initialized
int arr[10]{};          // ✅ fixed: array initialized
```

### 2. Equals Syntax → Brace Initialization

**For non-auto types:**
```cpp
int x = 5;              // → int x{5};
std::string s = "hi";   // → std::string s{"hi"};
```

**For auto types (opposite):**
```cpp
auto x{42};             // → auto x = 42;  (brace to equals)
auto* p{&value};        // → auto* p = &value;
const auto* cp{&value}; // → const auto* cp = &value;
```

**Why auto types are different:**
- `auto x{42}` can deduce to `std::initializer_list<int>` in some contexts
- `auto x = 42` is clearer and conventional for type deduction
- Brace initialization with auto can lead to surprising type deductions

### 3. Unsigned Suffix → `U` or `UL`

Codelint automatically adds the correct suffix based on type size:

| Type | Suffix | Example |
|------|--------|---------|
| `unsigned`, `unsigned int` | `U` | `unsigned u{1U}` |
| `uint64_t`, `unsigned long` | `UL` | `uint64_t val{42UL}` |

```cpp
unsigned u = 1;         // → unsigned u{1U};
uint64_t val = 42;      // → uint64_t val{42UL};
unsigned long big = 100; // → unsigned long big{100UL};
```

### 4. Bool from Integer → Error

```cpp
bool ret = Init();      // ❌ Error: assigning integer to bool is dangerous
bool flag = 1;          // ❌ Error: assigning integer to bool is dangerous

bool ok = true;         // ✅ OK: bool literal
bool status{false};     // ✅ OK: brace init with bool
```

### 5. Smart Skip List (Complete)

Codelint intelligently skips these cases where modification is unsafe or intentional:

| Category | Example | Reason |
|----------|---------|--------|
| **For loops** | `for (int i = 0; i < n; i++)` | C-style idiom |
| **Catch parameters** | `catch (const Exception& e)` | Exception parameter initialized by catch |
| **Catch block variables** | `std::string copy = msg;` inside catch | ✅ Still checked (not skipped) |
| **Macro definitions** | Variables inside `#define` | Cannot modify macros |
| **Auto with equals** | `auto x = getValue()` | ✅ Correct - no warning |
| **Auto with braces** | `auto x{42}` | ❌ Warn → `auto x = 42` |
| **Auto references** | `auto& ref = x` | ✅ Correct - uses `=` |
| **Extern declarations** | `extern int x;` | Definition elsewhere |
| **Union members** | `union { int a; }` | Union special handling |
| **Enum class** | `enum class Color { Red }` | Scoped enum |
| **Bitfields** | `int flags : 4;` | Bitfield special syntax |
| **References** | `int& ref = x;` | Must be bound at declaration |
| **Default arguments** | `void foo(int a = 10)` | Function signature |
| **Lambda captures** | `auto f = [](int x)` | Lambda context |
| **= {} syntax** | `int x = {};` | Already brace init |
| **Narrowing conversions** | `int x = 3.14;` | Cannot use {} (warns only) |
| **Type widening** | `float f = 5` | Safe implicit conversion |
| **initializer_list constructors** | `MyArray arr = 1` (class has `initializer_list<int>`) | Brace init would change constructor selection |

### 6. initializer_list Constructor Handling

Classes with `std::initializer_list` constructors require special care:

```cpp
class MyArray {
  MyArray(std::initializer_list<int> list);  // initializer_list constructor
  MyArray(int value);                         // regular constructor
};

MyArray arr = 1;   // ❌ SKIPPED - brace init would call initializer_list constructor
MyArray arr2{10};  // ✅ OK - explicit brace init, user's choice

std::string s = "hello";  // ✅ FIXED to std::string s{"hello"}
// std::string has initializer_list<char>, but "hello" is const char*, not initializer_list
```

### 7. Constructor Member Initialization

```cpp
class Widget {
  int id;
  std::string name;

  Widget() {}           // ❌ Warning: 'id', 'name' not initialized

  Widget() : id{}, name{} {}  // ✅ OK: all members initialized
};
  ```

## codelint-strict-bool-condition Features

This check enforces that only `bool` type expressions are used in condition statements, preventing dangerous implicit conversions from integers, pointers, and other types.

### Supported Language Versions

- C++14, C++17, C++20
- C++23 is **not supported** (may have incompatible features)

### What It Detects

| Statement Type | Checked Expression |
|----------------|-------------------|
| `if` | Condition expression |
| `while` | Loop condition |
| `for` | Loop condition (not initialization or increment) |
| `do-while` | Loop condition |
| Ternary `?:` | Condition expression |

### Invalid Conditions (Error)

```cpp
int status = 0;
if (status) {}              // ❌ Error: integer in condition
while (count--) {}          // ❌ Error: integer in condition
for (; ptr; ptr++) {}       // ❌ Error: pointer in condition
bool result = val ? true : false;  // ❌ Error: integer in ternary condition

// Even more dangerous:
if (strcmp(a, b)) {}        // ❌ Error: integer result in condition (strcmp returns int)
while (buffer = get()) {}   // ❌ Error: pointer result in condition (assignment returns pointer)
```

### Valid Conditions (OK)

```cpp
bool ready = true;
if (ready) {}               // ✅ OK: bool variable
while (!done) {}            // ✅ OK: bool expression
for (; active; ) {}         // ✅ OK: bool condition
bool result = enabled ? a : b;  // ✅ OK: bool in ternary condition

// Explicit comparisons produce bool:
if (status == 0) {}         // ✅ OK: comparison returns bool
while (ptr != nullptr) {}   // ✅ OK: comparison returns bool
if (strcmp(a, b) == 0) {}   // ✅ OK: explicit comparison to 0
```

### Why This Check?

| Problem | Risk |
|---------|------|
| `if (count)` | Treats 0 as false, non-zero as true - confusing |
| `if (ptr)` | Null check works, but intent unclear |
| `while (result = func())` | Assignment returns value, easy to miss `=` vs `==` |
| `if (strcmp(a, b))` | **Bug!** strcmp returns 0 for equal strings, non-zero for different |

Explicit comparisons make intent clear and prevent common bugs.

## Quick Start

```bash
# Build (using CMake presets)
# macOS
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default
# Linux
cmake --preset default
cmake --build build

# Run on your code
clang-tidy --load=build/lib/codelint-plugin.dylib \
            --checks='codelint-init,codelint-strict-bool-condition' \
            --fix \
            src/**/*.cpp
```

## AI Tool Integration

### For AI Assistants (Claude, Cursor, Copilot)

**Option 1: Use the wrapper script**

```bash
# Add to PATH
export PATH=/path/to/codelint/bin:$PATH

# AI can simply run
codelint --fix src/*.cpp
```

**Option 2: Use with .clang-tidy config**

```yaml
# .clang-tidy in project root
Checks: 'codelint-*'
HeaderFilterRegex: '.*'
```

**Option 3: Direct invocation**

```bash
clang-tidy --load=/path/to/codelint-plugin.so \
            --checks='codelint-init,codelint-strict-bool-condition' \
            --fix \
            -p build/compile_commands.json \
            src/**/*.cpp
```

### Tips for AI Assistants

1. **Always use `--fix`** for auto-fixable issues
2. **Manually review Error-level warnings** (int→bool conversion, uninitialized trivial types)
3. **Check constructor initializer lists** for member warnings
4. **Use `-p compile_commands.json`** for accurate analysis
5. **For strict-bool-condition**: add explicit comparisons (`==`, `!=`, `<`, etc.)

## Requirements

- LLVM/Clang 21+
- CMake 3.20+
- C++20 compiler

## License

MIT License
