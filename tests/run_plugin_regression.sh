#!/bin/bash

# Regression test for codelint clang-tidy plugin
# Tests: src files (with issues) -> clang-tidy --fix -> compare with fixed files

set -e
# set -x

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/init_checker"
TEST_BUILD_DIR="$PROJECT_ROOT/tests/CodeLintTest/build"
COMPILE_COMMANDS="$TEST_BUILD_DIR/compile_commands.json"

export PATH="/opt/homebrew/opt/llvm@21/bin:$PATH"

echo "========================================"
echo "Codelint Plugin Regression Test Suite"
echo "========================================"
echo ""

# Auto-detect plugin file based on platform
BUILD_DIR="$PROJECT_ROOT/build"
if [ -f "$BUILD_DIR/lib/codelint-plugin.so" ]; then
    PLUGIN="$BUILD_DIR/lib/codelint-plugin.so"
elif [ -f "$BUILD_DIR/lib/codelint-plugin.dylib" ]; then
    PLUGIN="$BUILD_DIR/lib/codelint-plugin.dylib"
else
    echo "ERROR: Plugin not found in $BUILD_DIR/lib/"
    echo "Please build the plugin first: cmake --build build"
    exit 1
fi

CLANG_TIDY=$(which clang-tidy 2>/dev/null || echo "")
if [ -z "$CLANG_TIDY" ]; then
    echo "ERROR: clang-tidy not found in PATH"
    exit 1
fi

echo "Using clang-tidy: $CLANG_TIDY"
echo "Using plugin: $PLUGIN"
echo "Using compile_commands: $COMPILE_COMMANDS"
echo ""

# Verify compile_commands.json exists
if [ ! -f "$COMPILE_COMMANDS" ]; then
    echo "ERROR: compile_commands.json not found at $COMPILE_COMMANDS"
    echo "Please build the test project first: cd tests/CodeLintTest/build && cmake .. && cmake --build ."
    exit 1
fi

TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Test: Apply clang-tidy --fix to source file and compare with expected fixed file
run_fix_test() {
    local test_name="$1"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $test_name"
    echo "------------------------------------------"

    if [ ! -f "$src_file" ]; then
        echo "SKIP: Source file not found: $src_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    if [ ! -f "$expected_file" ]; then
        echo "SKIP: Expected fixed file not found: $expected_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    # Create temp file for fix result
    local temp_file=$(mktemp /tmp/codelint_fix.XXXXXX.cpp)
    cp "$src_file" "$temp_file"

    # Apply clang-tidy --fix using compile_commands.json with proper compiler flags
    "$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks='codelint-init' --fix "$temp_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1 > /dev/null || true

    # Compare fixed result with expected
    if diff -q "$expected_file" "$temp_file" > /dev/null 2>&1; then
        echo "PASS: $test_name - Fixed output matches expected"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $test_name - Fixed output does NOT match expected"
        echo ""
        echo "Expected (fixed/${test_name}.cpp):"
        head -10 "$expected_file"
        echo ""
        echo "Got (after clang-tidy --fix):"
        head -10 "$temp_file"
        echo ""
        echo "Diff:"
        diff "$expected_file" "$temp_file" | head -20
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$temp_file"
}

# Test: Verify clang-tidy warning output matches expected
run_check_output_test() {
    local test_name="$1"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_output="$TEST_DIR/check-output/${test_name}.txt"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $test_name (check output)"
    echo "------------------------------------------"

    if [ ! -f "$expected_output" ]; then
        echo "SKIP: Expected output not found: $expected_output"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    # Run clang-tidy and save all output
    local temp_full="/tmp/codelint_full_$$.txt"
    local temp_output="/tmp/codelint_output_$$.txt"

    "$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks='codelint-init,-codelint-global,-codelint-singleton' "$src_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1 > "$temp_full" || true

    # Extract codelint-init warnings with context (the warning line + 3 subsequent format lines)
    awk '
        /warning: .* \[codelint-init\]/ { found=1; count=4; print; next }
        /warning: .* \[codelint-/ { found=0 }
        /Suppressed/ { found=0 }
        found && count > 0 { print; count-- }
        found && count == 0 { found=0 }
    ' "$temp_full" > "$temp_output"

    # Compare with expected output (strip trailing whitespace for flexible comparison)
    local temp_expected="/tmp/codelint_expected_$$.txt"
    sed 's/[[:blank:]]*$//' "$expected_output" > "$temp_expected"
    sed 's/[[:blank:]]*$//' "$temp_output" > "${temp_output}.stripped"

    if diff -q "$temp_expected" "${temp_output}.stripped" > /dev/null 2>&1; then
        echo "PASS: $test_name warning output matches expected"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $test_name warning output does NOT match expected"
        echo ""
        echo "Expected (check-output/${test_name}.txt):"
        head -6 "$expected_output"
        echo ""
        echo "Got (actual output):"
        head -6 "$temp_output"
        echo ""
        echo "Diff (ignoring trailing whitespace):"
        diff "$temp_expected" "${temp_output}.stripped" | head -10
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$temp_full" "$temp_output" "$temp_expected" "${temp_output}.stripped"
}

# Test: Verify source files have issues
run_source_issue_test() {
    local test_name="$1"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $test_name (source issues)"
    echo "------------------------------------------"

    if [ ! -f "$src_file" ]; then
        echo "SKIP: Source file not found: $src_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    # Skip if source and fixed files are identical (no issues expected)
    if [ -f "$expected_file" ] && diff -q "$src_file" "$expected_file" > /dev/null 2>&1; then
        echo "SKIP: Source and fixed files are identical (no issues expected)"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local output=$("$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks='codelint-init' "$src_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1) || true
    local issue_count=$(echo "$output" | grep -E "warning:.*codelint-init" | wc -l | awk '{print $1}')

    if [ "$issue_count" -gt 0 ]; then
        echo "PASS: $test_name detects $issue_count issue(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $test_name detects 0 issues (expected > 0)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# Test: Verify fixed files have no (or fewer) issues
run_fixed_issue_test() {
    local test_name="$1"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $test_name (fixed - no issues)"
    echo "------------------------------------------"

    if [ ! -f "$expected_file" ]; then
        echo "SKIP: Fixed file not found: $expected_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local output=$("$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks='codelint-init' "$expected_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1) || true
    local issue_count=$(echo "$output" | grep -E "warning:.*codelint-init" | wc -l | awk '{print $1}')

    if [ "$issue_count" -eq 0 ]; then
        echo "PASS: $test_name fixed file has 0 issues"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $test_name fixed file has $issue_count issues (expected 0)"
        echo "$output" | grep "warning:.*codelint-init" | head -3
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

echo "=== Phase 0: Verify check output ==="
echo ""

for src_file in "$TEST_DIR"/src/*.cpp; do
    if [ -f "$src_file" ]; then
        test_name=$(basename "$src_file" .cpp)
        run_check_output_test "$test_name"
    fi
done

echo ""
echo "=== Phase 1: Verify source files have issues ==="
echo ""

for test_file in "$TEST_DIR"/src/*.cpp; do
    if [ -f "$test_file" ]; then
        test_name=$(basename "$test_file" .cpp)
        run_source_issue_test "$test_name"
    fi
done

echo ""
echo "=== Phase 2: Verify fixed files have no issues ==="
echo ""

for test_file in "$TEST_DIR"/fixed/*.cpp; do
    if [ -f "$test_file" ]; then
        test_name=$(basename "$test_file" .cpp)
        run_fixed_issue_test "$test_name"
    fi
done

echo ""
echo "=== Phase 3: Apply --fix and compare with expected ==="
echo ""

for src_file in "$TEST_DIR"/src/*.cpp; do
    if [ -f "$src_file" ]; then
        test_name=$(basename "$src_file" .cpp)
        expected_file="$TEST_DIR/fixed/${test_name}.cpp"
        if [ -f "$expected_file" ]; then
            run_fix_test "$test_name"
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
