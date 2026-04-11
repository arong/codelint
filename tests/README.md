# Codelint Test Suite

This directory contains regression tests for the codelint clang-tidy plugin.

## Regression Tests

The main test suite uses clang-tidy's plugin infrastructure to verify that the codelint checks work correctly across different environments.

### Test Structure

```
tests/
├── run_plugin_regression.sh      # Main regression test runner
├── CodeLintTest/
│   └── src/init_checker/
│       ├── src/              # Source files WITH issues
│       │   ├── std.cpp
│       │   ├── integer.cpp
│       │   └── ...
│       ├── fixed/            # Expected output after clang-tidy --fix
│       │   ├── std.cpp
│       │   ├── integer.cpp
│       │   └── ...
│       └── check-output/     # Expected clang-tidy warnings
│           ├── std.txt
│           ├── integer.cpp
│           └── ...
```

### Running Tests

```bash
# Run all regression tests
bash tests/run_plugin_regression.sh

# Or use CMake target
cmake --build build --target test-all
```

## Test Phases

The regression test script runs 4 phases:

### Phase 0: Verify check output
- Runs clang-tidy on source files
- Compares warnings with `check-output/*.txt` expectations
- Validates diagnostic messages

### Phase 1: Verify source files have issues
- Ensures source files actually trigger the expected warnings
- Counts issues per file

### Phase 2: Verify fixed files have no issues
- Runs clang-tidy on `fixed/` files
- Expects ZERO warnings (all issues were fixed)

### Phase 3: Apply --fix and compare
- Applies clang-tidy --fix to source files
- Compares result with `fixed/` expectations
- Validates auto-fix functionality

## Adding New Tests

To add a new test case:

1. **Create source file** in `src/init_checker/src/your_test.cpp`
   - Include code that should trigger codelint checks

2. **Create expected fixed file** in `src/init_checker/fixed/your_test.cpp`
   - The expected output after clang-tidy --fix

3. **Create expected output** in `src/init_checker/check-output/your_test.txt`
   - The expected clang-tidy warning messages

4. **Run tests** to verify:
   ```bash
   bash tests/run_plugin_regression.sh
   ```

## Legacy Test Scripts

The following scripts are deprecated and may not exist:
- `test_find_global.sh` - Replaced by clang-tidy plugin tests
- `test_find_singleton.sh` - Replaced by clang-tidy plugin tests
- `test_check_init.sh` - Replaced by clang-tidy plugin tests
- `run_all_tests.sh` - Replaced by `run_plugin_regression.sh`
