#!/bin/bash
# bundle_libs.sh - Bundle clang-tidy and all required libraries for offline deployment
#
# Usage: ./bundle_libs.sh [output_dir]
#
# Creates a portable package with:
# - clang-tidy binary
# - codelint-plugin.so
# - All required LLVM/Clang shared libraries
#
# Target: Ubuntu 22.04 (no LLVM 21 installed, no network)

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
OUTPUT_DIR="${1:-${PROJECT_ROOT}/package-output}"
LLVM_VERSION="21"

# Get version from git or default
get_version() {
    if [ -n "$GITHUB_REF" ]; then
        echo "${GITHUB_REF#refs/tags/v}"
    elif git rev-parse --git-dir >/dev/null 2>&1; then
        git describe --tags --always 2>/dev/null || echo "dev"
    else
        echo "dev"
    fi
}

VERSION=$(get_version)
ARCH=$(uname -m)
PACKAGE_NAME="codelint-${VERSION}-linux-${ARCH}"
PACKAGE_DIR="${OUTPUT_DIR}/${PACKAGE_NAME}"

echo "========================================"
echo "Creating offline codelint package"
echo "========================================"
echo "Version: ${VERSION}"
echo "Architecture: ${ARCH}"
echo "Output: ${PACKAGE_DIR}"
echo ""

# Clean and create output directory
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}/bin"
mkdir -p "${PACKAGE_DIR}/lib"

# Step 1: Copy clang-tidy binary
echo "[1/6] Copying clang-tidy binary..."
CLANG_TIDY_FOUND=false
for candidate in \
    "/usr/bin/clang-tidy-${LLVM_VERSION}" \
    "/usr/lib/llvm-${LLVM_VERSION}/bin/clang-tidy" \
    "/usr/bin/clang-tidy"
do
    if [ -f "$candidate" ]; then
        cp "$candidate" "${PACKAGE_DIR}/bin/clang-tidy"
        CLANG_TIDY_FOUND=true
        echo "  Found: $candidate"
        break
    fi
done

if [ "$CLANG_TIDY_FOUND" = false ]; then
    echo "ERROR: clang-tidy not found!"
    echo "Searched locations:"
    echo "  - /usr/bin/clang-tidy-${LLVM_VERSION}"
    echo "  - /usr/lib/llvm-${LLVM_VERSION}/bin/clang-tidy"
    echo "  - /usr/bin/clang-tidy"
    echo "Please install: apt-get install clang-tidy-${LLVM_VERSION}"
    exit 1
fi
chmod +x "${PACKAGE_DIR}/bin/clang-tidy"

# Step 2: Copy codelint plugin
echo "[2/6] Copying codelint-plugin..."
PLUGIN_PATH="${BUILD_DIR}/lib/codelint-plugin.so"
if [ ! -f "${PLUGIN_PATH}" ]; then
    echo "ERROR: codelint-plugin.so not found at ${PLUGIN_PATH}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi
cp "${PLUGIN_PATH}" "${PACKAGE_DIR}/lib/codelint-plugin.so"

# Step 3: Collect library dependencies
echo "[3/6] Collecting library dependencies..."

# Function to copy a library and its symlink
copy_lib() {
    local lib_path="$1"
    local lib_name=$(basename "$lib_path")

    # Skip system libraries (should exist on target)
    local skip_patterns="libc.so libm.so libdl.so libpthread.so librt.so ld-linux"
    for pattern in $skip_patterns; do
        if [[ "$lib_name" == "$pattern"* ]]; then
            return
        fi
    done

    # Copy the library
    if [ ! -f "${PACKAGE_DIR}/lib/${lib_name}" ]; then
        cp -L "$lib_path" "${PACKAGE_DIR}/lib/${lib_name}" 2>/dev/null || true
        echo "  Copied: $lib_name"
    fi

    # Also copy symlink if exists (for version flexibility)
    local lib_dir=$(dirname "$lib_path")
    local symlink_name=$(ls -la "$lib_dir" | grep "$lib_name" | grep -v "^-" | awk '{print $NF}' | head -1)
    if [ -n "$symlink_name" ] && [ ! -f "${PACKAGE_DIR}/lib/$symlink_name" ]; then
        cp "$lib_dir/$symlink_name" "${PACKAGE_DIR}/lib/$symlink_name" 2>/dev/null || true
    fi
}

# Collect dependencies from clang-tidy
echo "  Analyzing clang-tidy dependencies..."
ldd "${PACKAGE_DIR}/bin/clang-tidy" 2>/dev/null | grep "=> /" | awk '{print $3}' | while read lib; do
    copy_lib "$lib"
done

# Collect dependencies from codelint plugin
echo "  Analyzing codelint-plugin dependencies..."
ldd "${PLUGIN_PATH}" 2>/dev/null | grep "=> /" | awk '{print $3}' | while read lib; do
    copy_lib "$lib"
done

# Step 4: Copy essential LLVM libraries (may not appear in ldd)
echo "[4/6] Copying essential LLVM libraries..."
LLVM_LIB_DIR="/usr/lib/llvm-${LLVM_VERSION}/lib"

# Core LLVM/Clang libraries
for lib in \
    "libclang-cpp.so.${LLVM_VERSION}" \
    "libclang-cpp.so.${LLVM_VERSION}.1" \
    "libLLVM-${LLVM_VERSION}.so" \
    "libclang.so.${LLVM_VERSION}" \
    "libclang.so.${LLVM_VERSION}.1" \
; do
    if [ -f "${LLVM_LIB_DIR}/${lib}" ]; then
        cp "${LLVM_LIB_DIR}/${lib}" "${PACKAGE_DIR}/lib/" 2>/dev/null || true
        echo "  Copied: $lib"
    fi
done

# Also create unversioned symlinks for compatibility
cd "${PACKAGE_DIR}/lib"

# Create symlinks only if target files exist
if [ -f "libclang-cpp.so.${LLVM_VERSION}" ]; then
    ln -sf libclang-cpp.so.${LLVM_VERSION} libclang-cpp.so
fi
if [ -f "libclang-cpp.so.${LLVM_VERSION}.1" ]; then
    ln -sf libclang-cpp.so.${LLVM_VERSION}.1 libclang-cpp.so
fi

if [ -f "libLLVM-${LLVM_VERSION}.so" ]; then
    ln -sf libLLVM-${LLVM_VERSION}.so libLLVM.so
fi

if [ -f "libclang.so.${LLVM_VERSION}" ]; then
    ln -sf libclang.so.${LLVM_VERSION} libclang.so
fi
if [ -f "libclang.so.${LLVM_VERSION}.1" ]; then
    ln -sf libclang.so.${LLVM_VERSION}.1 libclang.so
fi

cd - > /dev/null

# Step 5: Create wrapper script
echo "[5/6] Creating wrapper script..."
cat > "${PACKAGE_DIR}/bin/codelint" << 'WRAPPER_EOF'
#!/bin/bash
# codelint - Wrapper script for clang-tidy with codelint plugin
#
# Usage: codelint [clang-tidy options] [files]
#
# This wrapper automatically loads the codelint plugin and enables codelint-* checks.
# To use plain clang-tidy without plugin: codelint --raw [options]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_ROOT="$(dirname "$SCRIPT_DIR")"

# Set library path to use bundled libraries
export LD_LIBRARY_PATH="${PACKAGE_ROOT}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Check for --raw flag to skip plugin loading
if [ "$1" == "--raw" ]; then
    shift
    exec "${SCRIPT_DIR}/clang-tidy" "$@"
fi

# Default: load plugin with codelint checks
exec "${SCRIPT_DIR}/clang-tidy" \
    --load="${PACKAGE_ROOT}/lib/codelint-plugin.so" \
    --checks='codelint-*' \
    "$@"
WRAPPER_EOF
chmod +x "${PACKAGE_DIR}/bin/codelint"

# Step 5.1: Copy clang-tidy-diff.py for incremental scanning
echo "[5.1/6] Copying clang-tidy-diff.py..."
CLANG_TIDY_DIFF=""
for candidate in \
    "/usr/lib/llvm-${LLVM_VERSION}/share/clang/clang-tidy-diff.py" \
    "/usr/share/clang/clang-tidy-diff.py" \
    "/opt/homebrew/opt/llvm@${LLVM_VERSION}/share/clang/clang-tidy-diff.py"
do
    if [ -f "$candidate" ]; then
        cp "$candidate" "${PACKAGE_DIR}/bin/clang-tidy-diff.py"
        CLANG_TIDY_DIFF="$candidate"
        echo "  Found: $candidate"
        break
    fi
done

if [ -z "$CLANG_TIDY_DIFF" ]; then
    echo "  WARNING: clang-tidy-diff.py not found (optional)"
fi

# Step 5.2: Create codelint-diff wrapper for incremental scanning
echo "[5.2/6] Creating codelint-diff wrapper..."
cat > "${PACKAGE_DIR}/bin/codelint-diff" << 'DIFF_WRAPPER_EOF'
#!/usr/bin/env python3
"""codelint-diff - Incremental linting for modified lines only

Usage:
  git diff -U0 HEAD^ | codelint-diff -p1
  git diff -U0 | codelint-diff -p1 --fix
"""

import os
import subprocess
import sys
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    package_root = script_dir.parent

    diff_script = script_dir / "clang-tidy-diff.py"
    if not diff_script.exists():
        print("ERROR: clang-tidy-diff.py not found")
        sys.exit(1)

    plugin_path = package_root / "lib" / "codelint-plugin.so"
    if not plugin_path.exists():
        print("ERROR: codelint-plugin.so not found")
        sys.exit(1)

    clang_tidy = script_dir / "clang-tidy"

    import argparse
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-p", default=0)
    parser.add_argument("-path", dest="build_path", default=None)
    parser.add_argument("-fix", action="store_true")
    parser.add_argument("-j", type=int, default=1)
    args, extra = parser.parse_known_args()

    cmd = [
        sys.executable, str(diff_script),
        "-clang-tidy-binary", str(clang_tidy),
        "-p", str(args.p),
        "-load", str(plugin_path),
        "-checks", "codelint-*",
    ]

    if args.build_path:
        cmd.extend(["-path", args.build_path])
    if args.fix:
        cmd.append("-fix")
    if args.j > 1:
        cmd.extend(["-j", str(args.j)])
    cmd.extend(extra)

    result = subprocess.run(cmd)
    sys.exit(result.returncode)

if __name__ == "__main__":
    main()
DIFF_WRAPPER_EOF
chmod +x "${PACKAGE_DIR}/bin/codelint-diff"

# Step 5.3: Copy clang-apply-replacements (optional, for batch fixes)
echo "[5.3/6] Copying clang-apply-replacements..."
APPLY_REPL=""
for candidate in \
    "/usr/lib/llvm-${LLVM_VERSION}/bin/clang-apply-replacements" \
    "/usr/bin/clang-apply-replacements-${LLVM_VERSION}" \
    "/usr/bin/clang-apply-replacements" \
    "/opt/homebrew/opt/llvm@${LLVM_VERSION}/bin/clang-apply-replacements"
do
    if [ -f "$candidate" ]; then
        cp "$candidate" "${PACKAGE_DIR}/bin/clang-apply-replacements"
        APPLY_REPL="$candidate"
        echo "  Found: $candidate"
        break
    fi
done

if [ -z "$APPLY_REPL" ]; then
    echo "  WARNING: clang-apply-replacements not found (optional)"
fi

# Step 6: Create README
echo "[6/6] Creating README..."
cat > "${PACKAGE_DIR}/README.md" << 'README_EOF'
# Codelint - Offline clang-tidy Plugin Package

This package contains clang-tidy with the codelint plugin and all required libraries
for offline deployment on Ubuntu 22.04.

## Contents

- `bin/clang-tidy` - clang-tidy binary (LLVM 21)
- `bin/codelint` - Wrapper script that auto-loads the plugin
- `bin/codelint-diff` - Incremental scanner for modified lines only
- `bin/clang-tidy-diff.py` - LLVM's diff-based scanner
- `bin/clang-apply-replacements` - Batch fix applicator (optional)
- `lib/codelint-plugin.so` - Codelint clang-tidy plugin
- `lib/*.so` - Required LLVM/Clang libraries

## Requirements

- Ubuntu 22.04 (or compatible Linux distribution)
- No LLVM installation required
- No network access required

## Usage

### Quick Start (Recommended)

```bash
# Using the wrapper (auto-loads plugin)
./bin/codelint your_file.cpp

# With compilation database
./bin/codelint -p build/compile_commands.json src/*.cpp

# Apply fixes automatically
./bin/codelint --fix your_file.cpp
```

### Incremental Scanning (Recommended for CI/PRs)

Scan only modified lines instead of full project:

```bash
# Check changes in last commit
git diff -U0 HEAD^ | ./bin/codelint-diff -p1

# Check uncommitted changes
git diff -U0 | ./bin/codelint-diff -p1

# Check changes between branches
git diff -U0 main...HEAD | ./bin/codelint-diff -p1 -path build

# With auto-fix
git diff -U0 HEAD^ | ./bin/codelint-diff -p1 --fix
```

### Manual Usage

```bash
# Direct clang-tidy invocation
./bin/clang-tidy \
    --load=lib/codelint-plugin.so \
    --checks='codelint-*' \
    your_file.cpp

# Skip plugin loading
./bin/codelint --raw --checks='modernize-*' your_file.cpp
```

### Selective Checks

```bash
# Only initialization checks
./bin/codelint --checks='codelint-init' your_file.cpp

# Only global/singleton checks
./bin/codelint --checks='codelint-global,codelint-singleton' src/*.cpp
```

## Available Checks

| Check | Purpose | Auto-fix |
|-------|---------|----------|
| `codelint-init` | Variable initialization style | Yes |
| `codelint-global` | Global variable detection | No |
| `codelint-singleton` | Meyer's Singleton pattern | No |

## Troubleshooting

If you see "library not found" errors:

```bash
# Verify all libraries are present
ldd bin/clang-tidy | grep "not found"

# Check plugin dependencies
ldd lib/codelint-plugin.so | grep "not found"
```

## Version

Package version: ${VERSION}
LLVM version: ${LLVM_VERSION}
README_EOF

# Set RPATH on binaries to use bundled libraries
echo ""
echo "[Optional] Setting RPATH on binaries..."
if command -v patchelf >/dev/null 2>&1; then
    patchelf --set-rpath '$ORIGIN/../lib' "${PACKAGE_DIR}/bin/clang-tidy" 2>/dev/null || true
    patchelf --set-rpath '$ORIGIN' "${PACKAGE_DIR}/lib/codelint-plugin.so" 2>/dev/null || true
    echo "  RPATH set successfully"
else
    echo "  patchelf not available - relying on LD_LIBRARY_PATH wrapper"
fi

# Create tarball
echo ""
echo "Creating tarball..."
cd "${OUTPUT_DIR}"
tar -czvf "${PACKAGE_NAME}.tar.gz" "${PACKAGE_NAME}"
TARBALL_SIZE=$(du -h "${PACKAGE_NAME}.tar.gz" | cut -f1)

echo ""
echo "========================================"
echo "Package created successfully!"
echo "========================================"
echo "Package: ${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz"
echo "Size: ${TARBALL_SIZE}"
echo ""
echo "Directory structure:"
echo "${PACKAGE_DIR}/"
find "${PACKAGE_DIR}" -type f | sort | while read f; do
    size=$(du -h "$f" | cut -f1)
    rel_path=$(echo "$f" | sed "s|${PACKAGE_DIR}||")
    echo "  ${rel_path} (${size})"
done

echo ""
echo "To test:"
echo "  cd ${PACKAGE_DIR}"
echo "  ./bin/codelint --version"
echo "  ./bin/codelint --list-checks"
