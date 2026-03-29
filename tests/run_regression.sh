#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests"

echo "========================================"
echo "Codelint Regression Test Suite"
echo "========================================"
echo ""

CODELINT="$BUILD_DIR/codelint"
if [ ! -f "$CODELINT" ]; then
    echo "ERROR: codelint executable not found at $CODELINT"
    echo "Please build the project first: cd $BUILD_DIR && make codelint"
    exit 1
fi

COMPILE_DB="$TEST_DIR/CodeLintTest/build/compile_commands.json"
if [ ! -f "$COMPILE_DB" ]; then
    echo "ERROR: compile_commands.json not found at $COMPILE_DB"
    echo "Please regenerate: cd $TEST_DIR/CodeLintTest/build && cmake .. && make"
    exit 1
fi

TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

run_test() {
    local name="$1"
    local src_file="$2"
    local expected_file="$3"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $name"
    echo "------------------------------------------"
    
    # Run codelint with --fix
    local output_file=$(mktemp)
    "$CODELINT" lint "$src_file" --only=init --fix > "$output_file" 2>/dev/null || true
    
    # Compare with expected
    if diff -q "$expected_file" "$output_file" > /dev/null 2>&1; then
        echo "PASS: $name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $name"
        echo "  Expected: $expected_file"
        echo "  Got: $output_file (temporary)"
        diff "$expected_file" "$output_file" | head -20 || true
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    
    rm -f "$output_file"
    echo ""
}

run_json_test() {
    local name="$1"
    local src_file="$2"
    local expected_count="$3"
    local issue_type="$4"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $name (JSON)"
    echo "------------------------------------------"
    
    local output=$( "$CODELINT" -p "$TEST_DIR/CodeLintTest/build" lint --only=init "$src_file" --output-json 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)
issues = data.get('issues', [])
count = len([i for i in issues if i.get('type') == '$issue_type'])
print(count)
" 2>/dev/null || echo "0")
    
    if [ "$output" = "$expected_count" ]; then
        echo "PASS: $name - Found $output $issue_type issues (expected $expected_count)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $name - Found $output $issue_type issues (expected $expected_count)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    echo ""
}

echo "Running lint --only=init tests..."
echo ""

# Test 1: init_check.cpp
run_test "init_check.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/init_check.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/init_check.cpp"

# Test 2: integer.cpp
run_test "integer.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/integer.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/integer.cpp"

# Test 3: std.cpp
run_test "std.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/std.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/std.cpp"

# Test 4: exception.cpp (Bug #2 verification - catch variables)
run_test "exception.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/exception.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/exception.cpp"

echo ""

# Test 5: Verify fixed file reports 0 issues
echo "------------------------------------------"
echo "Test: Verify fixed files report 0 issues"
echo "------------------------------------------"
for fixed_file in "$TEST_DIR"/CodeLintTest/src/init_checker/fixed/*.cpp; do
    TEST_COUNT=$((TEST_COUNT + 1))
    issue_count=$("$CODELINT" -p "$TEST_DIR/CodeLintTest/build" lint --only=init "$fixed_file" 2>/dev/null | grep "Issue:" | wc -l)
    if [ "$issue_count" = "0" ]; then
        echo "PASS: $(basename $fixed_file) reports 0 issues"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $(basename $fixed_file) reports $issue_count issues (expected 0)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done
echo ""

# Test 6: Verify source file detects issues
echo "------------------------------------------"
echo "Test: Verify source files detect issues"
echo "------------------------------------------"
for src_file in "$TEST_DIR"/CodeLintTest/src/init_checker/src/*.cpp; do
    TEST_COUNT=$((TEST_COUNT + 1))
    issue_count=$("$CODELINT" -p "$TEST_DIR/CodeLintTest/build" lint --only=init "$src_file" 2>/dev/null | grep -i "issue" | wc -l)
    if [ "$issue_count" -gt 0 ]; then
        echo "PASS: $(basename $src_file) detects $issue_count issue(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $(basename $src_file) detects 0 issues (expected > 0)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done
echo ""

echo "========================================"
echo "Regression Test Summary"
echo "========================================"
echo "Total tests:  $TEST_COUNT"
echo "Passed:       $PASS_COUNT"
echo "Failed:       $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All regression tests PASSED!"
    exit 0
else
    echo "✗ $FAIL_COUNT test(s) FAILED!"
    exit 1
fi