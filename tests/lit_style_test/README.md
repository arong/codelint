# Lit-Style Testing for Codelint

This directory demonstrates a clang-tidy-style lit-based testing approach for codelint,
inspired by how clang-tidy tests its check implementations.

## Why This Approach?

The existing regression tests in `../CodeLintTest/` use a shell script-based approach
(`run_plugin_regression.sh`). This lit-style approach offers:

| Aspect | Shell Script Approach | Lit Approach |
|--------|----------------------|--------------|
| Test definition | Separate .txt files | Inline `// CHECK:` directives |
| Line references | Manual calculation | `[@LINE+N]` syntax |
| Multiple scenarios | Separate files | `-check-suffix=` support |
| Test discovery | Manual file enumeration | Automatic via lit |
| Standards testing | Hardcoded | `-std=c++11,c++14,c++17` |

## Directory Structure

```
lit_style_test/
├── check_codelint.py    # Test runner script (adapted from clang-tidy)
├── lit.cfg.py           # Lit configuration
├── lit.cfg.py.in        # CMake template for lit.cfg.py
├── CMakeLists.txt       # CMake integration
├── README.md            # This file
├── codelint-init-basic.cpp
├── codelint-lint-code-unsigned.cpp
├── codelint-strict-bool.cpp
└── codelint-multi-std.cpp
```

## Writing Tests

Test files use special `// RUN:` and `// CHECK-` directives:

```cpp
// RUN: %check_codelint %s codelint-init %t -- -std=c++17

int x;

void test() {
  int local_uninit;
}

// CHECK-MESSAGES: :[[@LINE-4]]:1: error: uninitialized trivial type 'x' [codelint-init]
// CHECK-MESSAGES: :[[@LINE-2]]:3: error: uninitialized trivial type 'local_uninit' [codelint-init]
```

### Directives

| Directive | Purpose |
|-----------|---------|
| `// RUN:` | How to run the test (substitutions: `%s`=source, `%t`=tmpdir) |
| `// CHECK-MESSAGES:` | Expected diagnostic output |
| `// CHECK-MESSAGES-NOT:` | Output that should NOT appear |
| `// CHECK-FIXES:` | Expected fixed code after `--fix` |
| `// CHECK-FIXES-NOT:` | Code that should NOT be changed |

### Line References

- `[@LINE]` - Current line
- `[@LINE+3]` - 3 lines below
- `[@LINE-2]` - 2 lines above

### Multiple Test Scenarios

Use `-check-suffix=` to run multiple scenarios from one file:

```cpp
// RUN: %check_clang_tidy -check-suffix=OP1 %s check-name %t -- -opt1
// RUN: %check_clang_tidy -check-suffix=OP2 %s check-name %t -- -opt2

// CHECK-MESSAGES-OP1: expected message for option 1
// CHECK-MESSAGES-OP2: expected message for option 2
```

### Multiple Standards

```cpp
// RUN: %check_codelint -std=c++11,c++14,c++17 %s codelint-lint-code %t -- -std=c++17
```

## Running Tests

### Option 1: Direct Python

```bash
python3 check_codelint.py test_file.cpp codelint-init /tmp \
    --plugin build/lib/codelint-plugin.dylib \
    --clang-tidy /opt/homebrew/opt/llvm@21/bin/clang-tidy
```

### Option 2: Via lit

```bash
# With CMake
cmake --build build --target check-codelint-lit

# Or directly with lit
lit.py -v .
```

### Option 3: Manual FileCheck

```bash
clang-tidy -p . --load=build/lib/codelint-plugin.dylib \
    --checks='codelint-init,codelint-lint-code,-codelint-global,*' \
    test_file.cpp -- -std=c++17 2>&1 | FileCheck test_file.cpp
```

## Comparison with Existing Tests

### Existing Approach (run_plugin_regression.sh)

```
CodeLintTest/src/init_checker/
├── src/integer.cpp          # Source with issues
├── fixed/integer.cpp        # Expected fixed output
└── check-output/integer.txt # Expected warnings
```

### Lit Approach (this directory)

```
lit_style_test/
└── codelint-init-basic.cpp  # Contains source + CHECK directives
```

## Can This Replace Existing Tests?

Not necessarily. Both approaches can coexist:

- **Existing tests**: Good for regression testing, comprehensive coverage
- **Lit tests**: Good for quick iteration, inline documentation, multiple scenarios

Consider lit-style for:
- New check development (TDD)
- Multiple edge cases in one file
- Standard variant testing
- Documentation inline with tests

Keep existing tests for:
- Full regression coverage
- Integration with CI/CD
- Complex multi-file scenarios

## Key Differences from clang-tidy

This implementation is **simplified** compared to clang-tidy's `check_clang_tidy.py`:

| Feature | clang-tidy | This Implementation |
|---------|------------|---------------------|
| FileCheck integration | Dual FileCheck (messages + fixes) | Simplified |
| YAML configs | Full support | Not included |
| Test plugins | CTTestTidyModule | Not needed |
| Header filtering | Complex patterns | Simple `.*` |
| Multiple RUN lines | Full lit syntax | Single RUN supported |

## Adapting clang-tidy's Approach

clang-tidy uses these key files:

1. **check_clang_tidy.py** - Filters CHECK lines, invokes FileCheck
2. **ClangTidyTest.h** - Unit test framework for gtest
3. **lit.cfg.py** - Lit configuration with substitutions

This implementation adapts #1 for codelint. For full adoption, you could:
1. Add unit tests using ClangTidyTest.h pattern
2. Enhance lit.cfg.py for more lit features
3. Add CHECK-FIXES verification
4. Implement test suffixes for multiple scenarios
