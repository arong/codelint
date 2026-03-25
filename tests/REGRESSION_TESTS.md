# CNDY Test Suite

This directory contains the test suite for the cndy C++ code analysis tool.

## Test Files

### Source Test Files
- `test_check_init.cpp` - Test cases for check_init command
- `test_check_init_braces.cpp` - Test cases for brace initialization handling
- `test_check_init_all_types.cpp` - Test cases for all variable types
- `test_find_global.cpp` - Test cases for find_global command
- `test_find_singleton.cpp` - Test cases for find_singleton command

### Test Scripts
- `smoke_test.sh` - Quick smoke tests for basic functionality
- `run_regression_tests.sh` - Comprehensive regression test suite
- `run_all_tests.sh` - Original test runner
- `test_check_init.sh` - Check_init specific tests
- `test_find_global.sh` - Find_global specific tests
- `test_find_singleton.sh` - Find_singleton specific tests

### Expected Output
- `expected/` - Directory containing expected output for regression tests

## Running Tests

### Quick Smoke Tests
```bash
./tests/smoke_test.sh
```

### Full Regression Tests
```bash
./tests/run_regression_tests.sh
```

### Individual Test Suites
```bash
./tests/test_check_init.sh
./tests/test_find_global.sh
./tests/test_find_singleton.sh
```

## Adding New Tests

1. Create a test source file in `tests/`
2. Add test cases to the appropriate test script
3. Generate expected output:
   ```bash
   ./build/cndy check_init tests/new_test.cpp > tests/expected/new_test.txt 2>&1
   ```
4. Update `run_regression_tests.sh` to include the new test
5. Run the regression tests to verify

## Test Organization

- **Smoke tests**: Fast, basic functionality checks
- **Regression tests**: Full output comparison tests
- **Unit tests**: Individual feature tests

## CI/CD Integration

The smoke tests can be run as part of CI/CD pipeline to quickly catch regressions:
```bash
./tests/smoke_test.sh || exit 1
```

For comprehensive testing before merging:
```bash
./tests/run_regression_tests.sh || exit 1
```