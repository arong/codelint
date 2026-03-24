#!/bin/bash

# Master test script for cndy

echo "========================================"
echo "Running all cndy tests"
echo "========================================"
echo ""

# Check if cndy is built
if [ ! -f "./build/cndy" ]; then
    echo "Error: cndy executable not found in ./build/"
    echo "Please run 'mkdir -p build && cd build && cmake .. && make' first"
    exit 1
fi

# Change to build directory for running tests
cd build

# Run individual test scripts
echo ""
echo "Running find_global tests..."
echo "------------------------------"
bash ../tests/test_find_global.sh
RESULT1=$?

echo ""
echo "Running find_singleton tests..."
echo "---------------------------------"
bash ../tests/test_find_singleton.sh
RESULT2=$?

echo ""
echo "Running check_init tests..."
echo "----------------------------"
bash ../tests/test_check_init.sh
RESULT3=$?

# Summary
echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "find_global tests:     $([ $RESULT1 -eq 0 ] && echo 'PASSED' || echo 'FAILED')"
echo "find_singleton tests:  $([ $RESULT2 -eq 0 ] && echo 'PASSED' || echo 'FAILED')"
echo "check_init tests:      $([ $RESULT3 -eq 0 ] && echo 'PASSED' || echo 'FAILED')"
echo ""

if [ $RESULT1 -eq 0 ] && [ $RESULT2 -eq 0 ] && [ $RESULT3 -eq 0 ]; then
    echo "All tests PASSED!"
    exit 0
else
    echo "Some tests FAILED!"
    exit 1
fi