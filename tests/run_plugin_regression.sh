#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/init_checker"
TEST_BUILD_DIR="$PROJECT_ROOT/tests/CodeLintTest/build"
COMPILE_COMMANDS="$TEST_BUILD_DIR/compile_commands.json"
COVERAGE_DIR="$PROJECT_ROOT/coverage_report"

COVERAGE_ENABLED=false

normalize_paths() {
    local input="$1"
    local output="$2"
    sed -E 's|^.+/codelint/tests/CodeLintTest/src/init_checker/|tests/CodeLintTest/src/init_checker/|g' "$input" > "$output"
}

detect_coverage_build() {
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        if grep -q "ENABLE_COVERAGE:BOOL=ON" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
            COVERAGE_ENABLED=true
            echo "Coverage build detected"
        fi
    fi
}

generate_coverage_report() {
    local LLVM_PROFDATA LLVM_COV

    LLVM_PROFDATA=$(which llvm-profdata 2>/dev/null || echo "")
    LLVM_COV=$(which llvm-cov 2>/dev/null || echo "")

    if [ -z "$LLVM_PROFDATA" ] || [ -z "$LLVM_COV" ]; then
        if [ -d "/opt/homebrew/opt/llvm@21/bin" ]; then
            LLVM_PROFDATA="/opt/homebrew/opt/llvm@21/bin/llvm-profdata"
            LLVM_COV="/opt/homebrew/opt/llvm@21/bin/llvm-cov"
        elif [ -d "/usr/lib/llvm-21/bin" ]; then
            LLVM_PROFDATA="/usr/lib/llvm-21/bin/llvm-profdata"
            LLVM_COV="/usr/lib/llvm-21/bin/llvm-cov"
        fi
    fi

    if [ -z "$LLVM_PROFDATA" ] || [ -z "$LLVM_COV" ]; then
        echo "WARNING: llvm-profdata/llvm-cov not found, skipping coverage report"
        return
    fi

    local profraw_files=$(ls "$PROJECT_ROOT"/*.profraw 2>/dev/null || echo "")
    if [ -z "$profraw_files" ]; then
        echo "WARNING: No coverage data collected"
        return
    fi

    echo "Merging coverage data..."
    "$LLVM_PROFDATA" merge -o "$COVERAGE_DIR/codelint.profdata" "$PROJECT_ROOT"/*.profraw

    echo "Generating coverage report..."
    "$LLVM_COV" report "$PLUGIN" \
        -instr-profile="$COVERAGE_DIR/codelint.profdata" \
        -ignore-filename-regex="(tests|build|CMakeFiles|^/opt/|^/usr/lib/llvm|^/Library/)" \
        > "$COVERAGE_DIR/coverage_summary.txt"

    "$LLVM_COV" show "$PLUGIN" \
        -instr-profile="$COVERAGE_DIR/codelint.profdata" \
        -format=html \
        -output-dir="$COVERAGE_DIR/html" \
        -ignore-filename-regex="(tests|build|CMakeFiles|^/opt/|^/usr/lib/llvm|^/Library/)" \
        -show-line-counts-or-regions \
        -show-expansions \
        -path-equivalence="$PROJECT_ROOT/src,"src

    echo ""
    cat "$COVERAGE_DIR/coverage_summary.txt"
    echo ""
    echo "HTML report: $COVERAGE_DIR/html/index.html"

    rm -f "$PROJECT_ROOT"/*.profraw
}


# Alias clang-tidy-21 to clang-tidy for compatibility
if command -v clang-tidy-21 >/dev/null 2>&1; then
    alias clang-tidy=clang-tidy-21
fi

detect_coverage_build

BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/init_checker"
TEST_BUILD_DIR="$PROJECT_ROOT/tests/CodeLintTest/build"
COMPILE_COMMANDS="$TEST_BUILD_DIR/compile_commands.json"

# Setup clang-tidy PATH based on platform
if command -v clang-tidy &> /dev/null; then
    # clang-tidy already in PATH
    :
elif command -v clang-tidy-21 &> /dev/null; then
    # Ubuntu apt.llvm.org installs clang-tidy-21
    alias clang-tidy=clang-tidy-21
elif [ -x "/usr/bin/clang-tidy-21" ]; then
    # Ubuntu with clang-tidy-21 in /usr/bin
    alias clang-tidy=clang-tidy-21
elif [ -d "/usr/lib/llvm-21/bin" ]; then
    # Ubuntu with LLVM 21 in /usr/lib/llvm-21/bin
    export PATH="/usr/lib/llvm-21/bin:$PATH"
elif [ -d "/opt/homebrew/opt/llvm@21/bin" ]; then
    # macOS with homebrew clang-tidy
    export PATH="/opt/homebrew/opt/llvm@21/bin:$PATH"
fi

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

CLANG_TIDY_BIN=clang-tidy
if command -v clang-tidy-21 > /dev/null 2>&1; then
    CLANG_TIDY_BIN=clang-tidy-21
fi

CLANG_TIDY=$(which $CLANG_TIDY_BIN 2>/dev/null || echo "")
if [ -z "$CLANG_TIDY" ]; then
    echo "ERROR: clang-tidy not found in PATH"
    exit 1
fi

echo "Using clang-tidy: $CLANG_TIDY"
echo "Using plugin: $PLUGIN"
echo "Using compile_commands: $COMPILE_COMMANDS"
echo ""

if [ "$COVERAGE_ENABLED" = true ]; then
    rm -f "$PROJECT_ROOT"/*.profraw
    export LLVM_PROFILE_FILE="$PROJECT_ROOT/codelint_%p.profraw"
    mkdir -p "$COVERAGE_DIR"
    echo "Coverage instrumentation enabled"
    echo "Profile files will be written to: $PROJECT_ROOT/codelint_*.profraw"
    echo ""
fi

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
        /warning:/ { found=0 }
        /Suppressed/ { found=0 }
        found && count > 0 { print; count-- }
        found && count == 0 { found=0 }
    ' "$temp_full" > "$temp_output"

    # Normalize paths for cross-platform comparison (macOS vs GitHub Actions paths)
    local temp_expected_norm="/tmp/codelint_expected_norm_$$.txt"
    local temp_output_norm="/tmp/codelint_output_norm_$$.txt"
    normalize_paths "$expected_output" "$temp_expected_norm"
    normalize_paths "$temp_output" "$temp_output_norm"

    # Compare normalized output (also strip trailing whitespace)
    local temp_expected_stripped="${temp_expected_norm}.stripped"
    local temp_output_stripped="${temp_output_norm}.stripped"
    sed 's/[[:blank:]]*$//' "$temp_expected_norm" > "$temp_expected_stripped"
    sed 's/[[:blank:]]*$//' "$temp_output_norm" > "$temp_output_stripped"

    if diff -q "$temp_expected_stripped" "$temp_output_stripped" > /dev/null 2>&1; then
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
        diff "$temp_expected_stripped" "$temp_output_stripped" | head -10
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$temp_full" "$temp_output" "$temp_expected_norm" "$temp_expected_stripped" "$temp_output_norm" "$temp_output_stripped"
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

    # Filter out bool-int warnings which are intentionally NOT auto-fixed (Error level)
    local filtered_output=$(echo "$output" | grep -v "assigning integer to bool")
    # Filter out narrowing conversion warnings which are intentionally NOT auto-fixed
    local filtered_output=$(echo "$filtered_output" | grep -v "narrowing conversion")
    local issue_count=$(echo "$filtered_output" | grep -E "warning:.*codelint-init" | wc -l | awk '{print $1}')

    if [ "$issue_count" -eq 0 ]; then
        echo "PASS: $test_name fixed file has 0 issues"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $test_name fixed file has $issue_count issues (expected 0)"
        echo "$filtered_output" | grep "warning:.*codelint-init" | head -3
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

if [ "$COVERAGE_ENABLED" = true ]; then
    echo "========================================"
    echo "Generating Coverage Report"
    echo "========================================"
    generate_coverage_report
fi

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All regression tests PASSED!"
    exit 0
else
    echo "✗ $FAIL_COUNT test(s) FAILED!"
    exit 1
fi
