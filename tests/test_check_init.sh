#!/bin/bash

# Test script for check_init command

echo "========================================"
echo "Testing check_init command"
echo "========================================"
echo ""

# Test 1: Plain text output
echo "Test 1: Plain text output"
echo "------------------------"
./cndy -p tests/test_check_init.cpp check_init
echo ""

# Test 2: JSON output
echo "Test 2: JSON output"
echo "-------------------"
./cndy -p tests/test_check_init.cpp --output-json check_init
echo ""

# Test 3: Expected issues
echo "Test 3: Expected initialization issues"
echo "---------------------------------------"
echo "Expected to find:"
echo "  - Uninitialized variables (a, b, c, d, e)"
echo "  - Variables with '=' initialization (f, g, h, i, j)"
echo "  - Unsigned without suffix (k, l, m, g, w)"
echo "  - Should NOT report proper brace init (n, o, p, q)"
echo "  - Should NOT report unsigned with suffix (r, s, t)"
echo "  - Should NOT report non-builtin types (str, vec)"
echo ""

# Test 4: Count issues by type
echo "Test 4: Issue count summary"
echo "--------------------------"
OUTPUT=$(./cndy -p tests/test_check_init.cpp --output-json check_init)
UNINIT_COUNT=$(echo "$OUTPUT" | grep -o '"uninitialized"' | wc -l)
EQUALS_COUNT=$(echo "$OUTPUT" | grep -o '"use_equals_init"' | wc -l)
UNSIGNED_COUNT=$(echo "$OUTPUT" | grep -o '"unsigned_without_suffix"' | wc -l)
TOTAL_COUNT=$(echo "$OUTPUT" | grep -o '"type"' | wc -l)

echo "Uninitialized issues: $UNINIT_COUNT"
echo "Use equals init issues: $EQUALS_COUNT"
echo "Unsigned without suffix issues: $UNSIGNED_COUNT"
echo "Total issues: $TOTAL_COUNT"
echo ""

echo "========================================"
echo "check_init tests completed"
echo "========================================"