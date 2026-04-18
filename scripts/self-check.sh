#!/bin/bash
# Self-check codelint source code using the codelint plugin
#
# IMPORTANT: This script is carefully designed to NOT modify system headers!
#
# Safety measures:
# 1. NO --fix-errors flag (that would modify system headers)
# 2. header-filter only matches project files ('codelint/.*')
# 3. Only checks src/ directory (excludes tests)
# 4. Two-step process: check first, then fix with confirmation

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LLVM_BIN="/opt/homebrew/opt/llvm@21/bin"
PLUGIN="${PROJECT_ROOT}/build/lib/codelint-plugin.dylib"
COMPILE_DB="${PROJECT_ROOT}/build/compile_commands.json"
SDK_PATH="/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo ""
    echo "========================================"
    echo "$1"
    echo "========================================"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

check_prerequisites() {
    print_header "Checking Prerequisites"

    # Check LLVM
    if [ ! -d "$LLVM_BIN" ]; then
        print_error "LLVM not found at $LLVM_BIN"
        echo "Install with: brew install llvm@21"
        exit 1
    fi
    print_success "LLVM found at $LLVM_BIN"

    # Check plugin
    if [ ! -f "$PLUGIN" ]; then
        print_error "Plugin not found at $PLUGIN"
        echo "Build with: cmake --build build --target codelint-plugin"
        exit 1
    fi
    print_success "Plugin found at $PLUGIN"

    # Check compile_commands.json
    if [ ! -f "$COMPILE_DB" ]; then
        print_error "compile_commands.json not found"
        echo "Generate with: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        exit 1
    fi
    print_success "compile_commands.json found"
}

# Step 1: Check only (no modifications)
run_check() {
    print_header "Step 1: Checking Source Code"

    echo "Checking src/ directory..."
    echo ""

    # SAFETY:
    # - NO --fix-errors (would modify system headers)
    # - header-filter='codelint/.*' only matches project headers
    # - Only src/**/*.cpp (excludes tests)

    "$LLVM_BIN/clang-tidy" \
        --load="$PLUGIN" \
        --checks='codelint-*' \
        -p "$PROJECT_ROOT/build" \
        --header-filter='codelint/.*' \
        --extra-arg="-isysroot/$SDK_PATH" \
        "${PROJECT_ROOT}/src/**/*.cpp" 2>&1 | \
        grep -E "^${PROJECT_ROOT}/src.*\[codelint-" | \
        tee /tmp/codelint_issues.txt || true

    ISSUE_COUNT=$(wc -l < /tmp/codelint_issues.txt | tr -d ' ')

    if [ "$ISSUE_COUNT" -eq 0 ]; then
        print_success "No codelint issues found!"
        return 0
    else
        print_warning "Found $ISSUE_COUNT issue(s)"
        return 1
    fi
}

# Step 2: Fix issues (with confirmation)
run_fix() {
    print_header "Step 2: Fixing Issues"

    if [ ! -s /tmp/codelint_issues.txt ]; then
        print_success "No issues to fix"
        return 0
    fi

    echo "Issues found:"
    cat /tmp/codelint_issues.txt
    echo ""

    read -p "Do you want to apply fixes? (y/N): " -n 1 -r
    echo ""

    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_warning "Skipping fixes"
        return 0
    fi

    echo "Applying fixes..."

    # SAFETY:
    # - NO --fix-errors (critical!)
    # - header-filter='codelint/.*' (only project headers)
    # - Only --fix (not --fix-errors)

    "$LLVM_BIN/clang-tidy" \
        --load="$PLUGIN" \
        --checks='codelint-init' \
        -p "$PROJECT_ROOT/build" \
        --header-filter='codelint/.*' \
        --fix \
        --extra-arg="-isysroot/$SDK_PATH" \
        "${PROJECT_ROOT}/src/**/*.cpp" 2>&1 | \
        grep -E "applied .* fixes" || true

    print_success "Fixes applied"

    # Format fixed files
    echo "Formatting fixed files..."
    "$LLVM_BIN/clang-format" -i "${PROJECT_ROOT}/src/**/*.cpp"
    print_success "Formatting complete"

    # Rebuild to verify
    echo "Rebuilding plugin..."
    cmake --build "${PROJECT_ROOT}/build" --target codelint-plugin 2>&1 | tail -3
}

# Main
main() {
    print_header "Codelint Self-Check Tool"

    echo ""
    echo "⚠️  SAFETY NOTICE:"
    echo "   This script will NOT modify system headers (LLVM, SDK, etc.)"
    echo "   - header-filter restricts to project files only"
    echo "   - NO --fix-errors flag (prevents system header modification)"
    echo "   - Only src/ directory is checked (tests excluded)"
    echo ""

    check_prerequisites

    run_check
    CHECK_RESULT=$?

    if [ $CHECK_RESULT -ne 0 ]; then
        run_fix
    fi

    print_header "Summary"

    if [ $CHECK_RESULT -eq 0 ]; then
        print_success "All checks passed!"
    else
        print_warning "Issues were found and may have been fixed"
        echo "Run this script again to verify fixes"
    fi
}

# Run
main "$@"
