# find_singleton Test Suite

## Overview
This test suite validates the `find_singleton` checker which detects singleton pattern implementations in C++ code.

## Test Scenarios

1. **basic_singleton** - Classic singleton with getInstance()
2. **singleton_static_local** - Static local instance singleton
3. **singleton_thread_safe** - Thread-safe double-checked locking
4. **singleton_meyers** - Meyer's singleton pattern
5. **singleton_deleted** - Singleton with deleted copy/move constructors
6. **singleton_ptr** - Singleton returning raw pointer
7. **singleton_ref** - Singleton returning reference
8. **singleton_template** - Template-based singleton
9. **singleton_not** - Non-singleton class (false positive test)
10. **singleton_multi** - Multiple singleton classes in one file

## Usage

### Run Tests
```bash
cd tests/CodeLintTest
mkdir -p build && cd build
cmake .. && make
./test_runner --test-suite=find_singleton
```

### Expected Output
Each test case produces a JSON file in the `expected/` directory containing:
- File path analyzed
- List of singleton classes found
- Detection method used

### Test Structure
- `src/` - Input C++ source files
- `expected/` - Expected JSON output files