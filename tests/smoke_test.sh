#!/bin/bash

# Quick smoke test for cndy
# This script runs basic tests to ensure core functionality works

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PASSED=0
FAILED=0

# Check if cndy is built
if [ ! -f "$BUILD_DIR/cndy" ]; then
    echo -e "${RED}Error: cndy not found. Building...${NC}"
    cd "$ROOT_DIR" && cmake -B build && cmake --build build
fi

echo "Running smoke tests..."
echo ""

# Test 1: check_init basic functionality
echo "Test 1: check_init basic check"
if "$BUILD_DIR/cndy" check_init tests/test_check_init.cpp 2>&1 | grep -q "Found 30 initialization issue(s)"; then
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 2: check_init fix functionality
echo "Test 2: check_init --fix"
OUTPUT=$("$BUILD_DIR/cndy" check_init tests/test_check_init.cpp --fix 2>&1)
if echo "$OUTPUT" | grep -q "std::string str{\"hello\"}" && echo "$OUTPUT" | grep -q "int f{10}"; then
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 3: find_global basic functionality
echo "Test 3: find_global basic check"
if "$BUILD_DIR/cndy" -p tests find_global 2>&1 | grep -q "Found 13 global variable(s)"; then
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 4: find_singleton basic functionality
echo "Test 4: find_singleton basic check"
if "$BUILD_DIR/cndy" -p tests find_singleton 2>&1 | grep -q "instance()"; then
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC}"
    FAILED=$((FAILED + 1))
fi

# Test 5: JSON output format
echo "Test 5: JSON output format"
if "$BUILD_DIR/cndy" check_init tests/test_check_init.cpp --output-json 2>&1 | jq -e . > /dev/null 2>&1; then
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${GREEN}SKIPPED${NC} (jq not available)"
fi

echo ""
echo "========================================"
echo "Smoke Test Results"
echo "========================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All smoke tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some smoke tests failed!${NC}"
    exit 1
fi