# CNDY Test Suite

This directory contains test files and scripts for the cndy C++ code analysis tool.

## Test Files

### test_find_global.cpp
Tests the `find_global` command with various global variable patterns:
- Basic global variables (int, const, unsigned)
- Static and thread_local variables
- Different types (float, double, char, bool)
- Unsigned with proper suffixes
- Class types (std::string, std::vector)

Expected result: Should find 13 global variables

### test_find_singleton.cpp
Tests the `find_singleton` command with various singleton patterns:
- Classic Meyer's singleton (`instance()`)
- Singleton with `getInstance()` naming
- Singleton in namespace
- Non-singleton static local variables (should NOT be detected)
- Static methods without local static (should NOT be detected)
- Methods returning value types (should NOT be detected)

Expected result: Should find 3 singleton instances:
- `Singleton::instance()`
- `Manager::getInstance()`
- `MyApp::Config::instance()`

### test_check_init.cpp
Tests the `check_init` command with various initialization patterns:
- Uninitialized variables
- Variables initialized with '=' (should report)
- Unsigned without suffix (should report)
- Proper brace initialization (should NOT report)
- Unsigned with proper suffix (should NOT report)
- Non-builtin types (should NOT report)

Expected result: Should find multiple initialization issues

## Test Scripts

### test_find_global.sh
Test script for the `find_global` command.
- Tests plain text output
- Tests JSON output
- Tests directory scanning

### test_find_singleton.sh
Test script for the `find_singleton` command.
- Tests plain text output
- Tests JSON output
- Tests directory scanning
- Lists expected singleton patterns

### test_check_init.sh
Test script for the `check_init` command.
- Tests plain text output
- Tests JSON output
- Counts issues by type (uninitialized, use_equals_init, unsigned_without_suffix)

### run_all_tests.sh
Master test script that runs all individual test scripts.
- Verifies cndy executable exists
- Runs all three test scripts
- Provides summary of test results

## Running Tests

### Run all tests:
```bash
bash tests/run_all_tests.sh
```

### Run individual tests:
```bash
# From project root
bash tests/test_find_global.sh
bash tests/test_find_singleton.sh
bash tests/test_check_init.sh
```

### Run commands directly:
```bash
# From build directory
./cndy -p ../tests/test_find_global.cpp find_global
./cndy -p ../tests/test_find_singleton.cpp find_singleton
./cndy -p ../tests/test_check_init.cpp check_init

# With JSON output
./cndy -p ../tests/test_find_global.cpp --output-json find_global
./cndy -p ../tests/test_find_singleton.cpp --output-json find_singleton
./cndy -p ../tests/test_check_init.cpp --output-json check_init
```

## Test Expectations

### find_global
- Should detect all global variables at translation unit level
- Should correctly identify const modifiers
- Should report variable types, names, locations, and initialization values
- Should NOT detect local variables

### find_singleton
- Should detect methods returning references named `instance` or `getInstance`
- Should correctly identify the class name
- Should NOT detect static methods returning values
- Should NOT detect methods without static local variables

### check_init
- Should detect uninitialized built-in type variables
- Should detect variables initialized with '='
- Should detect unsigned integers without 'U' suffix
- Should NOT report brace initialization
- Should NOT report unsigned integers with proper suffix
- Should NOT report non-built-in types (std::string, std::vector, etc.)
- Should NOT report issues in system headers (/usr/include/, /usr/lib/)