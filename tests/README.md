# Codelint Test Suite

This directory contains tests for the codelint clang-tidy plugin.

## Lit-Style Tests

Lit-style tests put source code, expected diagnostics, and expected fixes all in a single `.cpp` file using `// CHECK-MESSAGES:` and `// CHECK-FIXES:` annotations.

### Checker Directories

| Directory | Check Name | Count | What It Tests |
|-----------|------------|-------|---------------|
| `lit_style_tests_v2/init_checker` | `codelint-init` | 22 | Uninitialized variables, dangerous conversions |
| `lit_style_tests_v2/lint_code` | `codelint-lint-code` | 7 | Initialization style, unsigned suffix |
| `lit_style_tests_v2/lint_code_checker` | `codelint-lint-code` | 1 | Brace style conversion |
| `lit_style_tests_v2/global_checker` | `codelint-global` | 14 | Global variable detection |
| `lit_style_tests_v2/singleton_checker` | `codelint-singleton` | 11 | Singleton pattern detection |
| `lit_style_tests_v2/strict_bool_condition_checker` | `codelint-strict-bool-condition` | 7 | Bool condition enforcement |
| `lit_style_tests_v2/signed_to_unsigned_checker` | `codelint-signed-to-unsigned-return` | 1 | Signed→unsigned return detection |
| `lit_style_tests_v2/global_const_string_checker` | `codelint-global-const-string` | 2 | Global const string detection |
| `lit_style_tests_v2/local_var_naming_checker` | `codelint-local-var-naming` | 1 | Local variable naming convention |
| `lit_style_tests_v2/function_size_checker` | `codelint-function-size` | 1 | Oversized function detection |

### Running Lit Tests

```bash
# Run all lit tests
cmake --build build --target test-lit

# Or directly
bash tests/run_lit_tests.sh
```

### Adding a New Lit Test

Create a `.cpp` file in the appropriate checker directory:

```cpp
// RUN: %codelint %s codelint-init %t

void test() {
  int x;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: void test() {
// CHECK-FIXES:   int x{};
// CHECK-FIXES: }
```

**Key rules:**
- Use `[[@LINE-N]]` syntax for line references (never hardcode line numbers)
- One check type per file: `init_checker/` for `codelint-init`, `lint_code/` for `codelint-lint-code`
- Include `// CHECK-FIXES:` for expected auto-fix output

See `lit_style_tests_v2/README.md` for full details.
