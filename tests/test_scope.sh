#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CODELINT="$BUILD_DIR/codelint"

echo "========================================"
echo "Scope Mode Test Suite"
echo "========================================"
echo ""

TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

cleanup() {
    rm -rf "$PROJECT_ROOT/.test_scope"
}

trap cleanup EXIT

setup_test_repo() {
    rm -rf "$PROJECT_ROOT/.test_scope"
    mkdir -p "$PROJECT_ROOT/.test_scope"
    cd "$PROJECT_ROOT/.test_scope"
    git init
    git config user.name "Test"
    git config user.email "test@test.com"

    cp "$BUILD_DIR/codelint" .
}

run_test() {
    local name="$1"
    local expected="$2"
    local actual="$3"

    TEST_COUNT=$((TEST_COUNT + 1))

    if [ "$expected" = "$actual" ]; then
        echo "✓ PASS: $name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "✗ FAIL: $name"
        echo "  Expected: $expected"
        echo "  Actual: $actual"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

echo "Test 1: --scope all (default, reports all issues)"
echo "-------------------------------------------"
setup_test_repo

cat > test.cpp << 'EOF'
int old_var = 1;
int new_var = 2;
EOF
git add test.cpp
git commit -m "initial"

echo "int added_var = 3;" >> test.cpp

output=$(./codelint lint test.cpp 2>&1)
all_issues=$(echo "$output" | grep -c "info:" || echo 0)

run_test "Should report all issues with default (all)" "3" "$all_issues"
echo ""

echo "Test 2: --scope modified (file-level filtering)"
echo "-------------------------------------------"
output=$(./codelint --scope modified lint test.cpp 2>&1)
modified_output="$output"

has_running=$(echo "$output" | grep -c "Running init" || echo 0)
has_results=$(echo "$output" | grep -c "info:" || echo 0)

run_test "Should run checkers on modified file" "1" "$has_running"
echo ""

echo "Test 3: --scope all (explicit, backward compatibility)"
echo "-------------------------------------------"
output=$(./codelint --scope all lint test.cpp 2>&1)
all_explicit=$(echo "$output" | grep -c "info:" || echo 0)

run_test "Should report all issues with --scope all" "3" "$all_explicit"
echo ""

echo "Test 4: File skipping with --scope modified"
echo "-------------------------------------------"
cat > another.cpp << 'EOF'
int another_var = 4;
EOF

output=$(./codelint --scope modified lint test.cpp another.cpp 2>&1)
skip_count=$(echo "$output" | grep -c "Skipping unmodified file" || echo 0)

run_test "Should skip unmodified another.cpp" "1" "$skip_count"
echo ""

echo "========================================"
echo "Scope Test Summary"
echo "========================================"
echo "Total tests:  $TEST_COUNT"
echo "Passed:       $PASS_COUNT"
echo "Failed:       $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All scope tests PASSED!"
    exit 0
else
    echo "✗ $FAIL_COUNT test(s) FAILED!"
    exit 1
fi
