#!/bin/bash

# Test script for find_singleton command

echo "========================================"
echo "Testing find_singleton command"
echo "========================================"
echo ""

# Test 1: Plain text output
echo "Test 1: Plain text output"
echo "------------------------"
./cndy -p tests/test_find_singleton.cpp find_singleton
echo ""

# Test 2: JSON output
echo "Test 2: JSON output"
echo "-------------------"
./cndy -p tests/test_find_singleton.cpp --output-json find_singleton
echo ""

# Test 3: Expected singletons
echo "Test 3: Expected singleton detection"
echo "-------------------------------------"
echo "Expected to find:"
echo "  - Singleton::instance()"
echo "  - Manager::getInstance()"
echo "  - MyApp::Config::instance()"
echo "  - Should NOT detect Helper::getValue() (returns int)"
echo "  - Should NOT detect Calculator::add() (no static local)"
echo "  - Should NOT detect Factory::create() (returns value)"
echo ""

# Test 4: Directory scanning
echo "Test 4: Directory scanning"
echo "--------------------------"
./cndy -p tests find_singleton
echo ""

echo "========================================"
echo "find_singleton tests completed"
echo "========================================"