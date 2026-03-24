#!/bin/bash

# Test script for find_global command

echo "========================================"
echo "Testing find_global command"
echo "========================================"
echo ""

# Test 1: Plain text output
echo "Test 1: Plain text output"
echo "------------------------"
./cndy -p tests/test_find_global.cpp find_global
echo ""

# Test 2: JSON output
echo "Test 2: JSON output"
echo "-------------------"
./cndy -p tests/test_find_global.cpp --output-json find_global
echo ""

# Test 3: Test directory scanning
echo "Test 3: Directory scanning"
echo "--------------------------"
./cndy -p tests find_global
echo ""

echo "========================================"
echo "find_global tests completed"
echo "========================================"