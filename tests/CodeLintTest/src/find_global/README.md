# find_global Regression Tests

This directory contains regression tests for the `find_global` command.

## Structure

```
find_global/
├── src/         # Source C++ files to analyze
├── expected/    # Expected JSON output files
└── README.md    # This file
```

## Test Scenarios

### 1. basic_globals.cpp
Tests basic global variable types:
- `int` - basic integer
- `const int` - const integer
- `unsigned int` - unsigned integer

**Expected**: 3 global variables detected

### 2. static_globals.cpp
Tests static global variables:
- `static int` - static integer
- `static std::string` - static string

**Expected**: 2 static global variables detected

### 3. thread_local_globals.cpp
Tests thread-local storage:
- `thread_local double` - thread-local double

**Expected**: 1 thread_local variable detected

### 4. typed_globals.cpp
Tests various types:
- `float`, `double`, `char`, `bool`
- `std::vector<int>`, `std::string`

**Expected**: 6 global variables detected

### 5. class_globals.cpp
Tests class/struct instances:
- Custom struct instance
- Custom class instance

**Expected**: 2 global instances detected

### 6. extern_globals.cpp
Tests extern declarations (should be SKIPPED):
- `extern int` - declaration only, should NOT be detected
- `int` definition - should be detected

**Expected**: 1 variable detected (defined_var), extern declarations skipped

### 7. anon_namespace_globals.cpp
Tests anonymous namespace:
- Variables inside `namespace {}`

**Expected**: 2 variables detected

### 8. inline_globals.cpp
Tests C++17 inline variables:
- `inline int` - C++17 inline variable

**Expected**: 1 inline variable detected

### 9. template_globals.cpp
Tests template variables:
- Template variable with explicit specialization

**Expected**: Template variable instance detected

### 10. const_globals.cpp
Tests const/constexpr globals:
- `const int`, `const std::string`
- `constexpr int`

**Expected**: 3 const/constexpr variables detected

## Running Tests

```bash
# From project root
./build/codelint --output-json find_global tests/CodeLintTest/src/find_global/src/<file>.cpp
```

## JSON Output Schema

Each issue contains:
- `type`: "GLOBAL_VARIABLE"
- `severity`: "info"
- `checker`: "global"
- `name`: variable name
- `type_str`: C++ type string
- `file`: source file path
- `line`, `column`: location
- `description`: human-readable description
- `fixable`: false (detection only)
