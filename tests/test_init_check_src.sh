#!/bin/bash

# Regression test for init_check_src.cpp
# This test verifies that check_init correctly identifies initialization issues
# and that --fix produces the expected output with {} initialization

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"
TEST_FILE="$SCRIPT_DIR/init_check_src.cpp"
EXPECTED_FILE="$SCRIPT_DIR/expected/init_check_src_fix.cpp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Regression Test: init_check_src.cpp"
echo "========================================"
echo ""

# Check if cndy is built
if [ ! -f "$BUILD_DIR/cndy" ]; then
    echo -e "${RED}Error: cndy not found. Please build the project first.${NC}"
    exit 1
fi

# Check if test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${RED}Error: Test file not found: $TEST_FILE${NC}"
    exit 1
fi

# Test 1: Verify check_init detects issues
echo -n "Test 1: check_init detects initialization issues ... "
OUTPUT=$($BUILD_DIR/cndy check_init "$TEST_FILE" 2>&1)
ISSUE_COUNT=$(echo "$OUTPUT" | grep -c "initialization issue" || true)
if [ "$ISSUE_COUNT" -gt 0 ]; then
    echo -e "${GREEN}PASSED${NC} (found $ISSUE_COUNT issues)"
else
    echo -e "${RED}FAILED${NC} (no issues detected)"
    exit 1
fi

# Test 2: Verify --fix output matches expected
echo -n "Test 2: --fix output matches expected ... "
if [ ! -f "$EXPECTED_FILE" ]; then
    echo -e "${YELLOW}SKIPPED${NC} (expected file not found, generating...)"
    $BUILD_DIR/cndy check_init "$TEST_FILE" --fix > "$EXPECTED_FILE" 2>&1
    echo "Generated expected output at: $EXPECTED_FILE"
else
    TEMP_FILE=$(mktemp)
    $BUILD_DIR/cndy check_init "$TEST_FILE" --fix > "$TEMP_FILE" 2>&1
    if diff -u "$EXPECTED_FILE" "$TEMP_FILE" > /dev/null 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        rm -f "$TEMP_FILE"
    else
        echo -e "${RED}FAILED${NC}"
        echo ""
        echo "Diff between expected and actual output:"
        diff -u "$EXPECTED_FILE" "$TEMP_FILE" | head -50
        rm -f "$TEMP_FILE"
        exit 1
    fi
fi

# Test 3: Verify specific patterns in fixed output
echo -n "Test 3: Verify {} initialization pattern ... "
FIXED_OUTPUT=$($BUILD_DIR/cndy check_init "$TEST_FILE" --fix 2>&1)

# Check that uninitialized variables use {}
if echo "$FIXED_OUTPUT" | grep -q "char c1{}" && \
   echo "$FIXED_OUTPUT" | grep -q "int i1{}" && \
   echo "$FIXED_OUTPUT" | grep -q "float f1{}" && \
   echo "$FIXED_OUTPUT" | grep -q "bool b1{}"; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC} (missing {} initialization pattern)"
    exit 1
fi

# Test 4: Verify that '=' initialized variables are converted to {}
echo -n "Test 4: Verify '=' to '{}' conversion ... "
if echo "$FIXED_OUTPUT" | grep -q "int c_style1{10}" && \
   echo "$FIXED_OUTPUT" | grep -q "double c_style3{3.14}"; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC} (missing '=' to '{}' conversion)"
    exit 1
fi

# Test 5: Verify JSON output contains expected fields
echo -n "Test 5: Verify JSON output structure ... "
JSON_OUTPUT=$($BUILD_DIR/cndy check_init "$TEST_FILE" --output-json 2>&1)
if echo "$JSON_OUTPUT" | grep -q '"type":' && \
   echo "$JSON_OUTPUT" | grep -q '"name":' && \
   echo "$JSON_OUTPUT" | grep -q '"suggestion":'; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC} (JSON output missing expected fields)"
    exit 1
fi

echo ""
echo "========================================"
echo -e "${GREEN}All tests PASSED!${NC}"
echo "========================================"
