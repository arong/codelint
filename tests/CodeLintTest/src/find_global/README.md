# find_global Test Suite

## Overview
This test suite validates the `find_global` checker which detects global variable declarations in C++ code.

## Test Scenarios

1. **basic_global** - Simple global variable int declaration
2. **global_const** - Global const variable
3. **global_static** - Static global variable
4. **global_pointer** - Global pointer variable
5. **global_struct** - Global struct variable
6. **global_array** - Global array variable
7. **global_extern** - External global variable declaration
8. **global_class** - Global class instance
9. **namespace_global** - Global variable in namespace
10. **multiple_globals** - Multiple global variables in one file

## Usage

### Run Tests
```bash
cd tests/CodeLintTest
mkdir -p build && cd build
cmake .. && make
./test_runner --test-suite=find_global
```

### Expected Output
Each test case produces a JSON file in the `expected/` directory containing:
- File path analyzed
- List of global variables found
- Line numbers and column positions

### Test Structure
- `src/` - Input C++ source files
- `expected/` - Expected JSON output files