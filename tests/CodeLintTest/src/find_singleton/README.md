# find_singleton Regression Tests

This directory contains regression tests for the `find_singleton` command.

## Structure

```
find_singleton/
├── src/         # Source C++ files to analyze
├── expected/    # Expected JSON output files
└── README.md    # This file
```

## Test Scenarios

### 1. meyers_singleton.cpp
Classic Meyer's Singleton pattern:
- `static T& instance()` returning reference to static local

**Expected**: 1 singleton detected

### 2. getinstance_singleton.cpp
Alternative naming convention:
- `static T& getInstance()` returning reference to static local

**Expected**: 1 singleton detected

### 3. namespace_singleton.cpp
Singleton inside namespace:
- Class with `instance()` method in `namespace Config`

**Expected**: 1 singleton detected

### 4. false_positive_static.cpp
**False Positive Test** - Static local but NOT singleton:
- Returns value type (not reference)
- Function is not named `instance` or `getInstance`

**Expected**: 0 singletons detected (false positive avoidance)

### 5. false_positive_value.cpp
**False Positive Test** - Returns value, not reference:
- `static T create()` returning value type

**Expected**: 0 singletons detected

### 6. false_positive_pointer.cpp
**False Positive Test** - Returns pointer, not reference:
- `static T* get()` returning pointer to static local

**Expected**: 0 singletons detected

### 7. false_positive_ref.cpp
**False Positive Test** - Returns reference but not from static local:
- Returns reference to parameter
- No static local variable

**Expected**: 0 singletons detected

### 8. const_singleton.cpp
Const-correct singleton:
- `static const T& instance()` returning const reference

**Expected**: 1 singleton detected

### 9. thread_local_singleton.cpp
Thread-local singleton:
- `static thread_local T` inside instance method

**Expected**: 1 singleton detected

### 10. crtp_singleton.cpp
CRTP (Curiously Recurring Template Pattern) singleton:
- Template base class with `instance()` method
- Concrete class inherits from base

**Expected**: 1 singleton detected

## Running Tests

```bash
# From project root
./build/codelint --output-json find_singleton tests/CodeLintTest/src/find_singleton/src/<file>.cpp
```

## JSON Output Schema

Each issue contains:
- `type`: "SINGLETON_PATTERN"
- `severity`: "info"
- `checker`: "singleton"
- `name`: method name (e.g., "instance")
- `type_str`: return type (e.g., "Database&")
- `file`: source file path
- `line`, `column`: location
- `description`: human-readable description including class name
- `suggestion`: usage suggestion (e.g., "Database::instance()")
- `fixable`: false (detection only)

## False Positive Testing

Tests 4-7 are **critical quality tests** that verify the tool does NOT
report non-singleton patterns. These ensure:

1. Value returns are not singletons
2. Pointer returns are not singletons
3. Parameter returns are not singletons
4. Only `instance()` and `getInstance()` patterns are detected

All false positive tests should produce `{"issues": []}` (empty array).
