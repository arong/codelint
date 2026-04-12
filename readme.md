# Codelint - clang-tidy Plugin for C++ Initialization Best Practices

Codelint is a **clang-tidy plugin** that enforces modern C++ initialization best practices, helping you write safer and more consistent code.

## Why Codelint?

### Problems Codelint Solves

| Problem | Risk | Codelint Solution |
|---------|------|-------------------|
| Uninitialized variables | Undefined behavior, crashes | Auto-fix with `{}` initialization |
| `int x = 5` style | Less explicit, allows narrowing | Auto-fix to `int x{5}` |
| `bool b = 1` | Dangerous implicit conversion | **Error warning** - force explicit |
| Missing `U` suffix | Silent signed/unsigned bugs | Auto-fix with `U` suffix |
| Uninitialized class members | Partial initialization bugs | Constructor initializer check |

### Real-World Example

```cpp
// Before: Dangerous code that Codelint catches
int Init() { return 0; }

void process() {
  int count;              // ❌ uninitialized - undefined behavior
  int value = 3.14;       // ❌ narrowing conversion
  unsigned u = 100;       // ❌ missing U suffix
  bool ret = Init();      // ❌ ERROR: integer to bool is dangerous
}

// After: Safe code after Codelint auto-fix
void process() {
  int count{};            // ✅ explicitly initialized
  int value{3};           // ✅ explicit integer (user fixed narrowing)
  unsigned u{100U};       // ✅ proper U suffix
  bool ret{true};         // ✅ explicit bool (user fixed conversion)
}
```

## Checks

| Check | What It Does | Auto-fix |
|-------|--------------|----------|
| **codelint-init** | Variable initialization style | ✅ Yes |
| **codelint-global** | Global variable detection | ❌ No |
| **codelint-singleton** | Meyer's Singleton pattern detection | ❌ No |

## codelint-init Features

### 1. Uninitialized Variables → `{}`

```cpp
int x;                  // → int x{};
std::string str;        // → std::string str{};
int arr[10];            // → int arr[10]{};
```

### 2. Equals Syntax → Brace Initialization

```cpp
int x = 5;              // → int x{5};
std::string s = "hi";   // → std::string s{"hi"};
```

### 3. Unsigned Suffix → `U`

```cpp
unsigned u = 1;         // → unsigned u{1U};
uint64_t val = 42;      // → uint64_t val{42U};
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
| **Catch blocks** | `catch (int e)` | Exception handling |
| **Macro definitions** | Variables inside `#define` | Cannot modify macros |
| **Auto types** | `auto x = getValue()` | Type deduction required |
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

### 6. Constructor Member Initialization

```cpp
class Widget {
  int id;
  std::string name;

  Widget() {}           // ❌ Warning: 'id', 'name' not initialized

  Widget() : id{}, name{} {}  // ✅ OK: all members initialized
};
```

## Quick Start

```bash
# Build
cmake -B build -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm
cmake --build build

# Run on your code
clang-tidy --load=build/lib/codelint-plugin.dylib \
           --checks='codelint-init' \
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
           --checks='codelint-init' \
           --fix \
           -p build/compile_commands.json \
           src/**/*.cpp
```

### Tips for AI Assistants

1. **Always use `--fix`** for auto-fixable issues
2. **Manually review Error-level warnings** (int→bool conversion)
3. **Check constructor initializer lists** for member warnings
4. **Use `-p compile_commands.json`** for accurate analysis

## Requirements

- LLVM/Clang 21+
- CMake 3.20+
- C++20 compiler

## License

MIT License
