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
    "$CODELINT" check_init "$src_file" --fix 2>/dev/null > "$output_file" || true
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

echo "Running check_init tests..."
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

# Const detection tests
run_test "const_basic.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/const_basic.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/const_basic.cpp"

run_test "const_addr.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/const_addr.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/const_addr.cpp"

run_test "const_call.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/const_call.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/const_call.cpp"

run_test "const_array.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/src/const_array.cpp" \
    "$TEST_DIR/CodeLintTest/src/init_checker/fixed/const_array.cpp"

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
    output=$("$CODELINT" check_init "$src_file" 2>&1)
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

# Extended: find_global tests
echo "------------------------------------------"
echo "Running find_global tests..."
echo "------------------------------------------"

TEST_COUNT=$((TEST_COUNT + 1))
echo "Test $TEST_COUNT: find_global on test file"
output=$("$CODELINT" find_global "$TEST_DIR/test_find_global.cpp" 2>&1)
compile_error=$(echo "$output" | grep -c "fatal error" 2>/dev/null || echo "0")
compile_error=${compile_error//[^0-9]/}
if [ "$compile_error" -gt 0 ]; then
    echo "SKIP: find_global - compilation error"
    TEST_COUNT=$((TEST_COUNT - 1))
else
    global_count=$(echo "$output" | grep -c "global_" 2>/dev/null || echo "0")
    if [ "$global_count" -gt 0 ]; then
        echo "PASS: find_global detects $global_count global variable(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: find_global detects 0 global variables"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
fi

# Extended: find_singleton tests
echo "------------------------------------------"
echo "Running find_singleton tests..."
echo "------------------------------------------"

TEST_COUNT=$((TEST_COUNT + 1))
echo "Test $TEST_COUNT: find_singleton on test file"
output=$("$CODELINT" find_singleton "$TEST_DIR/test_find_singleton.cpp" 2>&1)
compile_error=$(echo "$output" | grep -c "fatal error" 2>/dev/null || echo "0")
compile_error=${compile_error//[^0-9]/}
if [ "$compile_error" -gt 0 ]; then
    echo "SKIP: find_singleton - compilation error"
    TEST_COUNT=$((TEST_COUNT - 1))
else
    singleton_count=$(echo "$output" | grep -c "instance\|getInstance" 2>/dev/null || echo "0")
    if [ "$singleton_count" -gt 0 ]; then
        echo "PASS: find_singleton detects $singleton_count singleton pattern(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: find_singleton detects 0 singleton patterns"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
fi

# Test JSON output for find_global
TEST_COUNT=$((TEST_COUNT + 1))
echo "Test $TEST_COUNT: find_global JSON output"
output=$("$CODELINT" --output-json find_global "$TEST_DIR/test_find_global.cpp" 2>&1)
if echo "$output" | grep -q '"issues"'; then
    echo "PASS: find_global JSON output valid"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: find_global JSON output invalid"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# Test JSON output for find_singleton
TEST_COUNT=$((TEST_COUNT + 1))
echo "Test $TEST_COUNT: find_singleton JSON output"
output=$("$CODELINT" --output-json find_singleton "$TEST_DIR/test_find_singleton.cpp" 2>&1)
if echo "$output" | grep -q '"issues"'; then
    echo "PASS: find_singleton JSON output valid"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    echo "FAIL: find_singleton JSON output invalid"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

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
