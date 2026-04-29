#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LIT_TEST_DIR="$PROJECT_ROOT/tests/lit_style_tests_v2"

cd "$PROJECT_ROOT"

echo "========================================"
echo "Codelint Lit-Style Test Suite"
echo "========================================"
echo ""

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

echo "Using plugin: $PLUGIN"
echo ""

CLANG_TIDY=""
for cmd in clang-tidy clang-tidy-21 clang-tidy-15; do
    if command -v "$cmd" &> /dev/null; then
        CLANG_TIDY="$(command -v "$cmd")"
        break
    fi
done

if [ -z "$CLANG_TIDY" ]; then
    for llvm_dir in /usr/lib/llvm-21/bin /usr/lib/llvm-15/bin /opt/homebrew/opt/llvm@21/bin /opt/homebrew/opt/llvm@15/bin; do
        if [ -x "$llvm_dir/clang-tidy" ]; then
            CLANG_TIDY="$llvm_dir/clang-tidy"
            break
        fi
    done
fi

if [ -z "$CLANG_TIDY" ]; then
    echo "ERROR: clang-tidy not found"
    exit 1
fi

echo "Using clang-tidy: $CLANG_TIDY"
echo ""

if ! command -v FileCheck &> /dev/null; then
    for llvm_dir in /usr/lib/llvm-21/bin /usr/lib/llvm-15/bin /opt/homebrew/opt/llvm@21/bin /opt/homebrew/opt/llvm@15/bin; do
        if [ -x "$llvm_dir/FileCheck" ]; then
            export PATH="$llvm_dir:$PATH"
            break
        fi
    done
fi

if ! command -v FileCheck &> /dev/null; then
    echo "ERROR: FileCheck not found (required for lit tests)"
    echo "Install LLVM tools or ensure LLVM bin directory is in PATH"
    exit 1
fi

echo "Using FileCheck: $(which FileCheck)"
echo ""

TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

CHECK_CODELINT="$LIT_TEST_DIR/check_codelint.py"

if [ ! -f "$CHECK_CODELINT" ]; then
    echo "ERROR: check_codelint.py not found at $CHECK_CODELINT"
    exit 1
fi

run_lit_test() {
    local test_file="$1"
    local checker_dir=$(basename $(dirname "$test_file"))
    local test_name=$(basename "$test_file" .cpp)

    TEST_COUNT=$((TEST_COUNT + 1))

    local check_name=""
    case "$checker_dir" in
        "init_checker") check_name="codelint-init" ;;
        "global_checker") check_name="codelint-global" ;;
        "singleton_checker") check_name="codelint-singleton" ;;
        "strict_bool_condition_checker") check_name="codelint-strict-bool-condition" ;;
        "signed_to_unsigned_checker") check_name="codelint-signed-to-unsigned-return" ;;
        "global_const_string_checker") check_name="codelint-global-const-string" ;;
        "lint_code_checker") check_name="codelint-lint-code" ;;
        *) check_name="$checker_dir" ;;
    esac

    echo "------------------------------------------"
    echo "Test $TEST_COUNT: $checker_dir/$test_name"
    echo "------------------------------------------"

    local temp_dir=$(mktemp -d)

    if python3 "$CHECK_CODELINT" \
        "$test_file" \
        "$check_name" \
        "$temp_dir" \
        --clang-tidy "$CLANG_TIDY" \
        --plugin "$PLUGIN" \
        --std c++17 2>&1; then
        echo "PASS: $checker_dir/$test_name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL: $checker_dir/$test_name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    rm -rf "$temp_dir"
}

CHECKER_DIRS=(
    "init_checker"
    "global_checker"
    "singleton_checker"
    "strict_bool_condition_checker"
    "signed_to_unsigned_checker"
    "global_const_string_checker"
    "lint_code_checker"
)

for CHECKER in "${CHECKER_DIRS[@]}"; do
    CHECKER_PATH="$LIT_TEST_DIR/$CHECKER"

    if [ ! -d "$CHECKER_PATH" ]; then
        echo "SKIP: Checker directory not found: $CHECKER_PATH"
        continue
    fi

    echo ""
    echo "========================================"
    echo "Testing Checker: $CHECKER"
    echo "========================================"
    echo ""

    for test_file in "$CHECKER_PATH"/*.cpp; do
        if [ -f "$test_file" ]; then
            run_lit_test "$test_file"
        fi
    done
done

echo ""
echo "========================================"
echo "Lit-Style Test Summary"
echo "========================================"
echo "Total tests:  $TEST_COUNT"
echo "Passed:       $PASS_COUNT"
echo "Failed:       $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All lit-style tests PASSED!"
    exit 0
else
    echo "✗ $FAIL_COUNT test(s) FAILED!"
    exit 1
fi
