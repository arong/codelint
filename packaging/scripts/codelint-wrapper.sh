#!/bin/bash
# codelint-wrapper.sh - Wrapper script for clang-tidy with codelint plugin
#
# This is a standalone wrapper that can be used when the package is extracted.
# It sets up LD_LIBRARY_PATH and loads the codelint plugin automatically.
#
# Usage: codelint-wrapper.sh [clang-tidy options] [files]
#
# For raw clang-tidy (no plugin): codelint-wrapper.sh --raw [options]

set -e

# Determine package root relative to script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# The package structure is:
# package_root/
#   bin/
#     codelint (this script)
#     clang-tidy
#   lib/
#     codelint-plugin.so
#     [other libs]

# If this script is in bin/, package root is parent
if [ -f "${SCRIPT_DIR}/clang-tidy" ]; then
    PACKAGE_ROOT="$(dirname "$SCRIPT_DIR")"
else
    # If this script is standalone, assume current directory
    PACKAGE_ROOT="${SCRIPT_DIR}"
fi

# Set library path
export LD_LIBRARY_PATH="${PACKAGE_ROOT}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Verify required files exist
if [ ! -f "${PACKAGE_ROOT}/bin/clang-tidy" ]; then
    echo "ERROR: clang-tidy not found at ${PACKAGE_ROOT}/bin/clang-tidy"
    exit 1
fi

if [ ! -f "${PACKAGE_ROOT}/lib/codelint-plugin.so" ]; then
    echo "ERROR: codelint-plugin.so not found at ${PACKAGE_ROOT}/lib/codelint-plugin.so"
    exit 1
fi

# Check for --raw flag
if [ "$1" == "--raw" ]; then
    shift
    exec "${PACKAGE_ROOT}/bin/clang-tidy" "$@"
fi

# Check for --version flag (fast response)
if [ "$1" == "--version" ]; then
    echo "codelint wrapper (LLVM 21 + codelint plugin)"
    "${PACKAGE_ROOT}/bin/clang-tidy" --version
    exit 0
fi

# Check for --help flag
if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "codelint - clang-tidy with codelint plugin"
    echo ""
    echo "Usage: codelint [options] [files]"
    echo ""
    echo "Options:"
    echo "  --raw          Use plain clang-tidy without plugin"
    echo "  --version      Show version information"
    echo "  --help         Show this help message"
    echo "  --list-checks  List available codelint checks"
    echo ""
    echo "This wrapper automatically loads the codelint plugin and enables"
    echo "'codelint-*' checks. Pass additional clang-tidy options as needed."
    echo ""
    echo "Examples:"
    echo "  codelint file.cpp                   # Check single file"
    echo "  codelint -p build src/*.cpp         # With compilation database"
    echo "  codelint --fix file.cpp             # Apply fixes automatically"
    echo "  codelint --checks='codelint-init'   # Only init checks"
    echo ""
    exit 0
fi

# Check for --list-checks
if [ "$1" == "--list-checks" ]; then
    "${PACKAGE_ROOT}/bin/clang-tidy" \
        --load="${PACKAGE_ROOT}/lib/codelint-plugin.so" \
        --checks='codelint-*' \
        --list-checks | grep codelint
    exit 0
fi

# Default behavior: load plugin with codelint checks
exec "${PACKAGE_ROOT}/bin/clang-tidy" \
    --load="${PACKAGE_ROOT}/lib/codelint-plugin.so" \
    --checks='codelint-*' \
    "$@"