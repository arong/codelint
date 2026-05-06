# Lit-Style Tests V2

This directory contains **lit-style tests** for the codelint clang-tidy plugin, inspired by LLVM's own test format.

## Test Statistics

| Checker Directory | Check Name | Files | What It Tests |
|-------------------|------------|-------|---------------|
| `init_checker` | `codelint-init` | 22 | Uninitialized variables, dangerous conversions (int→bool, narrowing) |
| `lint_code` | `codelint-lint-code` | 7 | Initialization style (`=`→`{}`), unsigned suffix (`U`/`UL`) |
| `lint_code_checker` | `codelint-lint-code` | 1 | Brace style conversion |
| `global_checker` | `codelint-global` | 14 | Global variable detection |
| `singleton_checker` | `codelint-singleton` | 11 | Meyer's Singleton pattern detection |
| `strict_bool_condition_checker` | `codelint-strict-bool-condition` | 7 | Bool-only condition enforcement |
| `signed_to_unsigned_checker` | `codelint-signed-to-unsigned-return` | 1 | Signed→unsigned return value detection |
| `global_const_string_checker` | `codelint-global-const-string` | 2 | Global const string detection |
| `local_var_naming_checker` | `codelint-local-var-naming` | 1 | Local variable naming convention |
| `function_size_checker` | `codelint-function-size` | 1 | Oversized function detection |
| **Total** | | **69** | |

### init_checker vs lint_code

The `init_checker` and `lint_code` directories test different aspects of initialization checking:

- **`init_checker/`** — Tests `codelint-init`: semantic correctness issues (error-level)
  - Uninitialized variables (trivial types = error, non-trivial = warning)
  - Dangerous conversions (int→bool, narrowing)
  - Constructor member initialization

- **`lint_code/`** — Tests `codelint-lint-code`: style enforcement (warning-level)
  - Equals → brace syntax (`int x = 5` → `int x{5}`)
  - Auto brace → equals (`auto x{42}` → `auto x = 42`)
  - Unsigned suffix (`unsigned u = 1` → `unsigned u{1U}`)
  - Skip rules (initializer_list constructors, for loops, etc.)

## Directory Structure

```
lit_style_tests_v2/
├── check_codelint.py           # Test runner (Python)
├── run_lit_tests.sh            # Shell-based test runner
├── lit.cfg.py                  # Lit configuration
├── lit.cfg.py.in               # CMake template
├── CMakeLists.txt              # CMake integration
├── README.md                   # This file
│
├── init_checker/               # 22 files — codelint-init tests
│   ├── integer.cpp             # Integer type initialization
│   ├── local_variables.cpp     # Local variable initialization
│   ├── member_init.cpp         # Class member initialization
│   ├── bitfield.cpp            # Bitfield member handling
│   └── ...
│
├── lint_code/                  # 7 files — codelint-lint-code tests
│   ├── auto_brace.cpp          # Auto type brace→equals conversion
│   ├── bool.cpp                # Bool variable style
│   ├── constructor_semantics.cpp # Constructor initialization style
│   ├── exception.cpp           # Exception object style
│   ├── initializer_list.cpp    # initializer_list constructor skip rule
│   ├── stl_fill_constructors.cpp # STL fill constructor skip rule
│   └── valarray_fill.cpp       # valarray fill constructor skip rule
│
├── global_checker/             # 14 files — codelint-global tests
├── singleton_checker/          # 11 files — codelint-singleton tests
├── strict_bool_condition_checker/ # 7 files
├── signed_to_unsigned_checker/    # 1 file
├── global_const_string_checker/   # 2 files
└── lint_code_checker/             # 1 file — brace_style.cpp
```

## Test Format

Each test file follows clang-tidy's lit pattern with `@LINE` references:

```cpp
// RUN: %codelint %s codelint-init %t

int global1;
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: variable is not initialized  [codelint-init]

int global3 = 1;
// No CHECK-MESSAGES — not tested by codelint-init

// === Expected Fixed Output ===
// CHECK-FIXES: int global1{};
```

### @LINE Reference Syntax

Line references use `[[@LINE-N]]` to avoid hardcoding line numbers:

| Syntax | Meaning |
|--------|---------|
| `[[@LINE-1]]` | The line before this CHECK comment |
| `[[@LINE-2]]` | Two lines before this CHECK comment |
| `[[@LINE]]` | The same line as this CHECK comment |

**Why @LINE?** When CHECK-MESSAGES lines are added or removed, fixed line numbers break. `@LINE` references stay correct regardless of edits.

## Running Tests

### Option 1: Via CMake (Recommended)

```bash
cmake --build build --target test-all
```

This runs both regression tests (CodeLintTest) AND lit-style tests.

### Option 2: Run only lit tests

```bash
cmake --build build --target test-lit
```

Or directly:

```bash
bash tests/run_lit_tests.sh
```

### Option 3: Direct Python (single test)

```bash
python3 tests/lit_style_tests_v2/check_codelint.py \
    tests/lit_style_tests_v2/init_checker/integer.cpp \
    codelint-init /tmp \
    --plugin build/lib/codelint-plugin.dylib \
    --clang-tidy /opt/homebrew/opt/llvm@21/bin/clang-tidy
```

## Adding New Tests

### For codelint-init (uninitialized variable checks)

Create a `.cpp` file in `init_checker/`:

```cpp
// RUN: %codelint %s codelint-init %t

void test() {
  int x;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]

  int y{}; // OK — no warning
}
```

### For codelint-lint-code (style checks)

Create a `.cpp` file in `lint_code/`:

```cpp
// RUN: %codelint %s codelint-lint-code %t

void test() {
  int x = 5;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]

  int y{5}; // OK — already using brace init
}
```

### Key Rules

1. **One check per directory**: `init_checker/` tests only `codelint-init`, `lint_code/` tests only `codelint-lint-code`
2. **Use `@LINE` references**: Never hardcode line numbers — use `[[@LINE-1]]`, `[[@LINE-2]]`, etc.
3. **Include CHECK-FIXES**: Add expected fixed output after `// === Expected Fixed Output ===`
4. **No mixed check types**: Don't put `codelint-init` and `codelint-lint-code` CHECK-MESSAGES in the same file

## Comparison with Original Tests

| Aspect | Original (CodeLintTest) | Lit-Style (lit_style_tests_v2) |
|--------|------------------------|--------------------------------|
| Source files | `src/*.cpp` | Inline in test file |
| Expected output | `check-output/*.txt` | `// CHECK-MESSAGES:` inline |
| Expected fixed | `fixed/*.cpp` | `// CHECK-FIXES:` inline |
| Test runner | `run_plugin_regression.sh` | `check_codelint.py` + FileCheck |
| Line references | Manual (fragile) | `[[@LINE-N]]` syntax (robust) |
| Check isolation | Mixed (init + lint-code) | Separated by directory |

## Original Tests Preserved

**The original tests are NOT deleted.** They remain at:

```
tests/CodeLintTest/src/
├── init_checker/
├── global_checker/
├── singleton_checker/
└── ...
```

Both test systems can coexist:
- **Original**: For comprehensive regression, CI/CD, complex scenarios
- **Lit-style**: For TDD, quick iteration, inline documentation
