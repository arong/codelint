# Lit-Style Tests V2 - Full Conversion

This directory contains **all existing CodeLintTest tests** converted to lit-style format.

## Converted Test Statistics

| Checker | Files Converted |
|---------|-----------------|
| init_checker | 29 |
| global_checker | 14 |
| singleton_checker | 11 |
| strict_bool_condition_checker | 7 |
| signed_to_unsigned_checker | 1 |
| global_const_string_checker | 2 |
| lint_code_checker | 1 |
| **Total** | **65** |

## Directory Structure

```
lit_style_tests_v2/
├── check_codelint.py           # Test runner (borrowed from lit_style_test)
├── convert_to_lit.py           # Conversion script (for reference)
├── lit.cfg.py                  # Lit configuration
├── lit.cfg.py.in               # CMake template
├── CMakeLists.txt              # CMake integration
├── README.md                   # This file
│
├── init_checker/               # 29 test files
│   ├── integer.cpp
│   ├── std.cpp
│   ├── bool.cpp
│   └── ...
│
├── global_checker/             # 14 test files
│   ├── basic_globals.cpp
│   ├── static_globals.cpp
│   └── ...
│
├── singleton_checker/          # 11 test files
│   ├── meyers_singleton.cpp
│   └── ...
│
├── strict_bool_condition_checker/  # 7 test files
│   ├── if_statements.cpp
│   └── ...
│
├── signed_to_unsigned_checker/     # 1 test file
│   └── signed_to_unsigned.cpp
│
├── global_const_string_checker/    # 2 test files
│   ├── basic.cpp
│   └── negative.cpp
│
└── lint_code_checker/              # 1 test file
    └── brace_style.cpp
```

## Test Format

Each test file follows clang-tidy's lit pattern:

```cpp
// RUN: %check_codelint %s codelint-init %t -- -std=c++17

int global1;
// CHECK-MESSAGES: :[@LINE]:5: error: variable is not initialized [codelint-init]

// === Expected Fixed Output ===
// CHECK-FIXES: int global1{};
```

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

### Option 3: Via GitHub Actions CI

```bash
# Build plugin first
cmake --build build

# Run lit tests
lit.py -v tests/lit_style_tests_v2/
```

### Option 2: Via CMake

```bash
cmake --build build --target check-codelint-lit-v2
```

### Option 3: Direct Python

```bash
python3 tests/lit_style_tests_v2/check_codelint.py \
    tests/lit_style_tests_v2/init_checker/integer.cpp \
    codelint-init /tmp \
    --plugin build/lib/codelint-plugin.dylib \
    --clang-tidy /opt/homebrew/opt/llvm@21/bin/clang-tidy
```

## Comparison with Original Tests

| Aspect | Original (CodeLintTest) | Lit-Style (lit_style_tests_v2) |
|--------|------------------------|--------------------------------|
| Source files | `src/*.cpp` | Inline in test file |
| Expected output | `check-output/*.txt` | `// CHECK-MESSAGES:` inline |
| Expected fixed | `fixed/*.cpp` | `// CHECK-FIXES:` inline |
| Test runner | `run_plugin_regression.sh` | `check_codelint.py` + lit |
| Line references | Manual | `[@LINE+N]` syntax |

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

## Manual Adjustments Needed

The conversion script may need manual adjustments for:

1. **Line references**: `[@LINE]` should be `[[@LINE]]` per clang-tidy convention
2. **CHECK-FIXES**: Some complex fixes may need manual refinement
3. **Multi-line messages**: May need to group related CHECK-MESSAGES

## Re-running Conversion

If you need to re-convert after updating original tests:

```bash
python3 tests/lit_style_tests_v2/convert_to_lit.py init_checker
python3 tests/lit_style_tests_v2/convert_to_lit.py global_checker
# ... etc
```

## Coverage Comparison

| Metric | Original Tests | Lit Tests |
|--------|----------------|-----------|
| Test count | 82 source files | 65 converted |
| Output verification | check-output/*.txt | inline CHECK-MESSAGES |
| Fix verification | fixed/*.cpp | inline CHECK-FIXES |
| Standards coverage | Hardcoded | Can use `-std=` variants |
