#!/bin/bash
#
# clang-tidy-wrapper.sh - Environment wrapper for bundled clang-tidy
#
# Sets up LD_LIBRARY_PATH/DYLD_LIBRARY_PATH for bundled LLVM libraries
# and invokes clang-tidy with correct environment.
#
# USAGE:
#   clang-tidy-wrapper.sh [CLANG-TIDY OPTIONS] FILES
#
# EXAMPLES:
#   # Basic usage
#   clang-tidy-wrapper.sh --checks=-*,bugprone-* src/main.cpp
#
#   # With plugin
#   clang-tidy-wrapper.sh --load=lib/codelint-plugin.so src/main.cpp
#
#   # With compile_commands.json
#   clang-tidy-wrapper.sh -p build src/**/*.cpp
#

set -e

# Determine script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKILL_DIR="$(dirname "$SCRIPT_DIR")"

# Determine platform
PLATFORM="$(uname -s)"
ARCH="$(uname -m)"

# Find clang-tidy binary
find_clang_tidy() {
    # Bundled binary paths (platform-specific)
    if [[ "$PLATFORM" == "Darwin" ]]; then
        if [[ "$ARCH" == "arm64" ]]; then
            BUNDLED="$SKILL_DIR/binaries/darwin-arm64/bin/clang-tidy"
        else
            BUNDLED="$SKILL_DIR/binaries/darwin-x64/bin/clang-tidy"
        fi
    else
        BUNDLED="$SKILL_DIR/binaries/linux-x64/bin/clang-tidy"
    fi

    if [[ -x "$BUNDLED" ]]; then
        echo "$BUNDLED"
        return
    fi

    # Alternative bundled locations
    for path in "$SKILL_DIR/bin/clang-tidy" "$SCRIPT_DIR/../bin/clang-tidy"; do
        if [[ -x "$path" ]]; then
            echo "$path"
            return
        fi
    done

    # System clang-tidy (LLVM 21 priority)
    for version in 21 20 19 18; do
        for prefix in "/usr/bin" "/usr/lib/llvm-$version/bin" "/opt/homebrew/opt/llvm@$version/bin"; do
            if [[ -x "$prefix/clang-tidy" ]]; then
                echo "$prefix/clang-tidy"
                return
            fi
        done
    done

    # Default system clang-tidy
    if command -v clang-tidy &> /dev/null; then
        echo "$(command -v clang-tidy)"
        return
    fi

    echo ""
}

# Find library directory
find_lib_dir() {
    # Bundled library paths (platform-specific)
    if [[ "$PLATFORM" == "Darwin" ]]; then
        if [[ "$ARCH" == "arm64" ]]; then
            BUNDLED_LIB="$SKILL_DIR/binaries/darwin-arm64/lib"
        else
            BUNDLED_LIB="$SKILL_DIR/binaries/darwin-x64/lib"
        fi
    else
        BUNDLED_LIB="$SKILL_DIR/binaries/linux-x64/lib"
    fi

    if [[ -d "$BUNDLED_LIB" ]] && [[ -n "$(ls -A "$BUNDLED_LIB"/*.so 2>/dev/null || ls -A "$BUNDLED_LIB"/*.dylib 2>/dev/null)" ]]; then
        echo "$BUNDLED_LIB"
        return
    fi

    # Alternative bundled locations
    for path in "$SKILL_DIR/lib" "$SCRIPT_DIR/../lib"; do
        if [[ -d "$path" ]] && [[ -n "$(ls -A "$path"/*.so 2>/dev/null || ls -A "$path"/*.dylib 2>/dev/null)" ]]; then
            echo "$path"
            return
        fi
    done

    echo ""
}

# Find clang-tidy
CLANG_TIDY=$(find_clang_tidy)

if [[ -z "$CLANG_TIDY" ]]; then
    echo "ERROR: clang-tidy not found"
    echo ""
    echo "Install options:"
    echo "  macOS:  brew install llvm@21"
    echo "  Linux:  apt install clang-tidy-21"
    echo "  Or use bundled binary from clang-tidy-skill package"
    exit 1
fi

# Find library directory
LIB_DIR=$(find_lib_dir)

# Build environment
if [[ "$PLATFORM" == "Darwin" ]]; then
    if [[ -n "$LIB_DIR" ]]; then
        export DYLD_LIBRARY_PATH="$LIB_DIR${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    fi
else
    if [[ -n "$LIB_DIR" ]]; then
        export LD_LIBRARY_PATH="$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
fi

# Run clang-tidy
exec "$CLANG_TIDY" "$@"
