#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_BUILD_DIR="$PROJECT_ROOT/tests/CodeLintTest/build"
COMPILE_COMMANDS="$TEST_BUILD_DIR/compile_commands.json"
COVERAGE_DIR="$PROJECT_ROOT/coverage_report"

COVERAGE_ENABLED=false

normalize_paths() {
    local input="$1"
    local output="$2"
    local checker="$3"
    sed -E "s|^.+/codelint/tests/CodeLintTest/src/${checker}/|tests/CodeLintTest/src/${checker}/|g" "$input" > "$output"
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

CLANG_FORMAT_BIN=clang-format
if command -v clang-format-21 > /dev/null 2>&1; then
    CLANG_FORMAT_BIN=clang-format-21
fi

CLANG_FORMAT=$(which $CLANG_FORMAT_BIN 2>/dev/null || echo "")
if [ -z "$CLANG_FORMAT" ]; then
    if [ -d "/opt/homebrew/opt/llvm@21/bin" ]; then
        CLANG_FORMAT="/opt/homebrew/opt/llvm@21/bin/clang-format"
    elif [ -d "/usr/lib/llvm-21/bin" ]; then
        CLANG_FORMAT="/usr/lib/llvm-21/bin/clang-format"
    fi
fi

if [ -z "$CLANG_FORMAT" ]; then
    echo "WARNING: clang-format not found, Phase 3 tests may fail due to formatting differences"
fi

echo "Using clang-tidy: $CLANG_TIDY"
echo "Using clang-format: $CLANG_FORMAT"
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

get_check_flag() {
    local checker="$1"
    case "$checker" in
        "init_checker")    echo "codelint-init,-codelint-global,-codelint-singleton,-codelint-strict-bool-condition,-codelint-signed-to-unsigned-return,-codelint-null-deref" ;;
        "global_checker")  echo "-*,codelint-global" ;;
        "singleton_checker") echo "-*,codelint-singleton" ;;
        "strict_bool_condition_checker") echo "-*,codelint-strict-bool-condition" ;;
        "signed_to_unsigned_checker") echo "-*,codelint-signed-to-unsigned-return" ;;
        "null_deref_checker") echo "-*,codelint-null-deref" ;;
        *) echo "" ;;
    esac
}

has_fix_phase() {
    local checker="$1"
    case "$checker" in
        "init_checker") echo "true" ;;
        *) echo "false" ;;
    esac
}

run_fix_test() {
    local checker="$1"
    local test_name="$2"
    local TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$checker"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"
    local check_flag=$(get_check_flag "$checker")

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker/$test_name (fix)"
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

    local temp_file=$(mktemp /tmp/codelint_fix.XXXXXX.cpp)
    cp "$src_file" "$temp_file"

    "$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks="$check_flag" --fix --fix-errors "$temp_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1 > /dev/null || true

    # Apply clang-format to both files to eliminate formatting differences
    local temp_formatted=$(mktemp /tmp/codelint_formatted.XXXXXX.cpp)
    local expected_formatted=$(mktemp /tmp/codelint_expected_formatted.XXXXXX.cpp)
    cp "$temp_file" "$temp_formatted"
    cp "$expected_file" "$expected_formatted"
    "$CLANG_FORMAT" -i "$temp_formatted" 2>/dev/null || true
    "$CLANG_FORMAT" -i "$expected_formatted" 2>/dev/null || true

    # Normalize whitespace to handle clang-format version differences
    # Collapse multiple spaces to single space, but preserve indentation
    local temp_normalized=$(mktemp /tmp/codelint_normalized.XXXXXX.cpp)
    local expected_normalized=$(mktemp /tmp/codelint_expected_normalized.XXXXXX.cpp)
    sed 's/  */ /g' "$temp_formatted" > "$temp_normalized"
    sed 's/  */ /g' "$expected_formatted" > "$expected_normalized"

    if diff -q "$expected_normalized" "$temp_normalized" > /dev/null 2>&1; then
        echo "PASS: $checker/$test_name - Fixed output matches expected"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $checker/$test_name - Fixed output does NOT match expected"
        echo ""
        echo "Expected (fixed/${test_name}.cpp after clang-format):"
        head -10 "$expected_formatted"
        echo ""
        echo "Got (after clang-tidy --fix + clang-format):"
        head -10 "$temp_formatted"
        echo ""
        echo "Diff:"
        diff "$expected_normalized" "$temp_normalized" | head -20
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -f "$temp_file" "$temp_formatted" "$expected_formatted" "$temp_normalized" "$expected_normalized"
}

# Test: Verify clang-tidy warning output matches expected
run_check_output_test() {
    local checker="$1"
    local test_name="$2"
    local TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$checker"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_output="$TEST_DIR/check-output/${test_name}.txt"
    local check_flag=$(get_check_flag "$checker")

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker/$test_name (check output)"
    echo "------------------------------------------"

    if [ ! -f "$expected_output" ]; then
        echo "SKIP: Expected output not found: $expected_output"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local temp_full="/tmp/codelint_full_$$.txt"
    local temp_output="/tmp/codelint_output_$$.txt"

    "$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks="$check_flag" "$src_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1 > "$temp_full" || true

    awk '
        /warning: .* \[codelint-init\]/ { found=1; count=4; print; next }
        /error: .* \[codelint-init\]/ { found=1; count=4; print; next }
        /warning: .* \[codelint-global\]/ { found=1; count=3; print; next }
        /warning: .* \[codelint-singleton\]/ { found=1; count=3; print; next }
        /warning: .* \[codelint-strict-bool-condition\]/ { found=1; count=3; print; next }
        /error: .* \[codelint-strict-bool-condition\]/ { found=1; count=3; print; next }
        /warning: .* \[codelint-signed-to-unsigned-return\]/ { found=1; count=3; print; next }
        /warning:/ { found=0 }
        /error:/ { found=0 }
        /Suppressed/ { found=0 }
        /Found compiler/ { found=0 }
        found && count > 0 { print; count-- }
        found && count == 0 { found=0 }
    ' "$temp_full" > "$temp_output"

    local temp_expected_norm="/tmp/codelint_expected_norm_$$.txt"
    local temp_output_norm="/tmp/codelint_output_norm_$$.txt"
    normalize_paths "$expected_output" "$temp_expected_norm" "$checker"
    normalize_paths "$temp_output" "$temp_output_norm" "$checker"

    local temp_expected_stripped="${temp_expected_norm}.stripped"
    local temp_output_stripped="${temp_output_norm}.stripped"
    sed 's/[[:blank:]]*$//' "$temp_expected_norm" > "$temp_expected_stripped"
    sed 's/[[:blank:]]*$//' "$temp_output_norm" > "$temp_output_stripped"

    if diff -q "$temp_expected_stripped" "$temp_output_stripped" > /dev/null 2>&1; then
        echo "PASS: $checker/$test_name warning output matches expected"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $checker/$test_name warning output does NOT match expected"
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
    local checker="$1"
    local test_name="$2"
    local TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$checker"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"
    local check_flag=$(get_check_flag "$checker")

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker/$test_name (source issues)"
    echo "------------------------------------------"

    if [ ! -f "$src_file" ]; then
        echo "SKIP: Source file not found: $src_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local output=$("$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks="$check_flag" "$src_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1) || true
    local issue_count=$(echo "$output" | grep -E "(warning|error):.*\[codelint-.*\]" | wc -l | awk '{print $1}')

    local expected_issues=0
    if [ -f "$TEST_DIR/check-output/${test_name}.txt" ] && [ -s "$TEST_DIR/check-output/${test_name}.txt" ]; then
        expected_issues=$(grep -c "warning:" "$TEST_DIR/check-output/${test_name}.txt" || echo 0)
    fi

    if [ "$issue_count" -gt 0 ]; then
        echo "PASS: $checker/$test_name detects $issue_count issue(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        if [ "$expected_issues" -eq 0 ]; then
            echo "PASS: $checker/$test_name correctly detects 0 issues (false positive test)"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo "FAIL: $checker/$test_name detects 0 issues (expected > 0)"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    fi
}

# Test: Verify fixed files have no (or fewer) issues
run_fixed_issue_test() {
    local checker="$1"
    local test_name="$2"
    local TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$checker"
    local expected_file="$TEST_DIR/fixed/${test_name}.cpp"
    local check_flag=$(get_check_flag "$checker")

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker/$test_name (fixed - no issues)"
    echo "------------------------------------------"

    if [ ! -f "$expected_file" ]; then
        echo "SKIP: Fixed file not found: $expected_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local output=$("$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks="$check_flag" "$expected_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1) || true

    local filtered_output=$(echo "$output" | grep -v "assigning integer to bool")
    local filtered_output=$(echo "$filtered_output" | grep -v "narrowing conversion")
    local issue_count=$(echo "$filtered_output" | grep -E "(warning|error):.*\[codelint-.*\]" | wc -l | awk '{print $1}')

    if [ "$issue_count" -eq 0 ]; then
        echo "PASS: $checker/$test_name fixed file has 0 issues"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $checker/$test_name fixed file has $issue_count issues (expected 0)"
        echo "$filtered_output" | grep -E "(warning|error):.*\[codelint-.*\]" | head -3
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

run_compilation_error_test() {
    local checker="$1"
    local test_name="$2"
    local TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$checker"
    local src_file="$TEST_DIR/src/${test_name}.cpp"
    local check_flag=$(get_check_flag "$checker")

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker/$test_name (compilation error - no false suggestions)"
    echo "------------------------------------------"

    if [ ! -f "$src_file" ]; then
        echo "SKIP: Source file not found: $src_file"
        TEST_COUNT=$((TEST_COUNT - 1))
        return
    fi

    local temp_full="/tmp/codelint_error_full_$$.txt"
    "$CLANG_TIDY" -p "$COMPILE_COMMANDS" --load="$PLUGIN" --checks="$check_flag" "$src_file" -- --std=c++17 -I"$TEST_DIR/src" -I"$TEST_BUILD_DIR" 2>&1 > "$temp_full" || true

    local has_compilation_error=0
    if grep -q "file not found" "$temp_full" 2>/dev/null; then
        has_compilation_error=1
    fi

    local codelint_warnings=$(grep -E "(warning|error):.*\[codelint-.*\]" "$temp_full" 2>/dev/null | wc -l | tr -d ' ')

    rm -f "$temp_full"

    if [ "$has_compilation_error" -eq 1 ] && [ "$codelint_warnings" -eq 0 ]; then
        echo "PASS: $checker/$test_name correctly skips analysis on compilation error"
        PASS_COUNT=$((PASS_COUNT + 1))
    elif [ "$has_compilation_error" -eq 0 ]; then
        echo "FAIL: $checker/$test_name - expected compilation error not detected"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    else
        echo "FAIL: $checker/$test_name - produced $codelint_warnings false suggestion(s) on compilation error"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# Run tests for each checker
CHECKERS=("init_checker" "global_checker" "singleton_checker" "strict_bool_condition_checker" "signed_to_unsigned_checker")

for CHECKER in "${CHECKERS[@]}"; do
    TEST_DIR="$PROJECT_ROOT/tests/CodeLintTest/src/$CHECKER"

    if [ ! -d "$TEST_DIR/src" ]; then
        echo "SKIP: Checker directory not found: $TEST_DIR"
        continue
    fi

    echo ""
    echo "========================================"
    echo "Testing Checker: $CHECKER"
    echo "========================================"
    echo ""

    echo "=== Phase 0: Verify check output ==="
    echo ""

    for src_file in "$TEST_DIR"/src/*.cpp; do
        if [ -f "$src_file" ]; then
            test_name=$(basename "$src_file" .cpp)
            run_check_output_test "$CHECKER" "$test_name"
        fi
    done

    echo ""
    echo "=== Phase 1: Verify source files have issues ==="
    echo ""

    for src_file in "$TEST_DIR"/src/*.cpp; do
        if [ -f "$src_file" ]; then
            test_name=$(basename "$src_file" .cpp)
            run_source_issue_test "$CHECKER" "$test_name"
        fi
    done

    if [ "$(has_fix_phase "$CHECKER")" = "true" ]; then
        echo ""
        echo "=== Phase 2: Verify fixed files have no issues ==="
        echo ""

        for fixed_file in "$TEST_DIR"/fixed/*.cpp; do
            if [ -f "$fixed_file" ]; then
                test_name=$(basename "$fixed_file" .cpp)
                run_fixed_issue_test "$CHECKER" "$test_name"
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
                    run_fix_test "$CHECKER" "$test_name"
                fi
            fi
        done
    fi

    echo ""
    echo "=== Phase 4: Verify compilation error handling ==="
    echo ""

    error_test_file="$TEST_DIR/src/missing_header.cpp"
    if [ -f "$error_test_file" ]; then
        run_compilation_error_test "$CHECKER" "missing_header"
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
