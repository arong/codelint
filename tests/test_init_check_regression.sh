#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"
TEST_FILE="$SCRIPT_DIR/init_check_src.cpp"
EXPECTED_DIR="$SCRIPT_DIR/expected"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PASS=0
FAIL=0

check_output() {
    local test_name="$1"
    local actual_file="$2"
    local expected_file="$3"
    
    if [ ! -f "$expected_file" ]; then
        echo -e "${RED}FAIL${NC}: $test_name - Expected file not found: $expected_file"
        FAIL=$((FAIL + 1))
        return
    fi
    
    if diff -q "$expected_file" "$actual_file" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}: $test_name"
        PASS=$((PASS + 1))
        rm -f "$actual_file"
    else
        echo -e "${RED}FAIL${NC}: $test_name"
        echo "  Diff:"
        diff -u "$expected_file" "$actual_file" | head -20
        FAIL=$((FAIL + 1))
    fi
}

echo "=========================================="
echo "Regression Test: init_check_src.cpp"
echo "=========================================="
echo ""

if [ ! -f "$BUILD_DIR/cndy" ]; then
    echo -e "${RED}Error: cndy not found${NC}"
    exit 1
fi

if [ ! -f "$TEST_FILE" ]; then
    echo -e "${RED}Error: Test file not found${NC}"
    exit 1
fi

TMP_DIR=$(mktemp -d)

echo "Test: check_init plain output"
ACTUAL="$TMP_DIR/plain_output.txt"
$BUILD_DIR/cndy check_init "$TEST_FILE" > "$ACTUAL" 2>&1
check_output "plain output" "$ACTUAL" "$EXPECTED_DIR/init_check_src_output.txt"

echo "Test: check_init --fix"
ACTUAL="$TMP_DIR/fix_output.cpp"
$BUILD_DIR/cndy check_init "$TEST_FILE" --fix > "$ACTUAL" 2>&1
check_output "--fix" "$ACTUAL" "$EXPECTED_DIR/init_check_src_fix.cpp"

echo "Test: check_init --output-json"
ACTUAL="$TMP_DIR/json_output.json"
$BUILD_DIR/cndy check_init "$TEST_FILE" --output-json > "$ACTUAL" 2>&1
check_output "--output-json" "$ACTUAL" "$EXPECTED_DIR/init_check_src_output.json"

rm -rf "$TMP_DIR"

echo ""
echo "=========================================="
echo "Results: $PASS passed, $FAIL failed"
echo "=========================================="

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    exit 1
fi
