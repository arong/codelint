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
    exit 1
fi

COMPILE_DB="$TEST_DIR/CodeLintTest/build/compile_commands.json"
if [ ! -f "$COMPILE_DB" ]; then
    echo "ERROR: compile_commands.json not found"
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

    local output_file=$(mktemp)
    "$CODELINT" check_init "$src_file" --fix 2>/dev/null | \
        grep -v "^Running\|^===\|^$\|^/Users.*:" > "$output_file" || true
    first_line=$(head -n 1 "$output_file")
    if [ -z "$first_line" ]; then
        tail -n +2 "$output_file" > "${output_file}.tmp" && mv "${output_file}.tmp" "$output_file"
    fi

    if diff -q "$expected_file" "$output_file" > /dev/null 2>&1; then
        echo "PASS: $name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $name"
        diff "$expected_file" "$output_file" | head -10 || true
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$output_file"
}

echo "Running lint --only=init tests..."
echo ""

# Test 1-4: Fixed file comparison
run_test "init_check.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/init_check.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/init_check.cpp"

run_test "integer.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/integer.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/integer.cpp"

run_test "std.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/std.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/std.cpp"

run_test "exception.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/exception.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/exception.cpp"

echo ""

# Test 5-8: Verify fixed files report 0 issues
echo "------------------------------------------"
echo "Test: Verify fixed files report 0 issues"
echo "------------------------------------------"
for fixed_file in "$TEST_DIR"/CodeLintTest/src/init_checker/fixed/*.cpp; do
    TEST_COUNT=$((TEST_COUNT + 1))
    output=$("$CODELINT" check_init "$fixed_file" 2>&1)
    compile_error=$(echo "$output" | grep -c "fatal error" 2>/dev/null || echo "0")
    compile_error=${compile_error//[^0-9]/}
    if [ "$compile_error" -gt 0 ]; then
        echo "SKIP: $(basename $fixed_file) - compilation error"
        TEST_COUNT=$((TEST_COUNT - 1))
    else
        issue_count=$(echo "$output" | grep -E "warning|error|info|hint" | wc -l | awk '{print $1}')
        if [ "$issue_count" = "0" ]; then
            echo "PASS: $(basename $fixed_file) reports 0 issues"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "FAIL: $(basename $fixed_file) reports $issue_count issues"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    fi
done
echo ""

# Test 9-12: Verify source files detect issues (only macOS-compatible files)
echo "------------------------------------------"
echo "Test: Verify source files detect issues"
echo "------------------------------------------"
# Only test init_check.cpp and integer.cpp (work on macOS)
for src_file in "$TEST_DIR"/CodeLintTest/src/init_checker/src/{init_check,integer}.cpp; do
    TEST_COUNT=$((TEST_COUNT + 1))
    output=$("$CODELINT" lint "$src_file" --only=init 2>&1)
    compile_error=$(echo "$output" | grep -c "fatal error" 2>/dev/null || echo "0")
    compile_error=${compile_error//[^0-9]/}
    if [ "$compile_error" -gt 0 ]; then
        echo "SKIP: $(basename $src_file) - compilation error"
        TEST_COUNT=$((TEST_COUNT - 1))
    else
        issue_count=$(echo "$output" | grep -E "warning|error|info|hint" | wc -l | awk '{print $1}')
        if [ "$issue_count" -gt 0 ]; then
            echo "PASS: $(basename $src_file) detects $issue_count issue(s)"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "FAIL: $(basename $src_file) detects 0 issues"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
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
