#!/bin/bash
# bundle_plugin.sh - Bundle codelint plugin for LLVM 15
#
# Creates a minimal package containing only the codelint plugin with a symlink.
# Target environment is assumed to have LLVM 15 (including clang-tidy-15) installed.
#
# Usage: ./bundle_plugin.sh
#
# Output: codelint-plugin-VERSION-linux-ARCH-llvm15.tar.gz
#   - codelint-plugin-VERSION-linux-ARCH-llvm15.so
#   - codelint-plugin.so (symlink to the above)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
OUTPUT_DIR="${PROJECT_ROOT}/package-output"
LLVM_VERSION="15"

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
PLATFORM=$(uname -s)

if [ "$PLATFORM" = "Darwin" ]; then
    PLATFORM_NAME="darwin"
    PLUGIN_EXT="dylib"
else
    PLATFORM_NAME="linux"
    PLUGIN_EXT="so"
fi

PACKAGE_NAME="codelint-plugin-${VERSION}-${PLATFORM_NAME}-${ARCH}-llvm${LLVM_VERSION}"
PACKAGE_DIR="${OUTPUT_DIR}/${PACKAGE_NAME}"

echo "========================================"
echo "Creating codelint plugin package (LLVM ${LLVM_VERSION})"
echo "========================================"
echo "Version: ${VERSION}"
echo "Platform: ${PLATFORM_NAME}-${ARCH}"
echo "Output: ${PACKAGE_DIR}"
echo ""

# Clean and create output directory
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

# Find the plugin
PLUGIN_PATH="${BUILD_DIR}/lib/codelint-plugin.${PLUGIN_EXT}"
if [ ! -f "${PLUGIN_PATH}" ]; then
    echo "ERROR: codelint-plugin.${PLUGIN_EXT} not found at ${PLUGIN_PATH}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

# Copy plugin with versioned name
PLUGIN_NAME_VERSIONED="codelint-plugin-${VERSION}-${PLATFORM_NAME}-${ARCH}-llvm${LLVM_VERSION}.${PLUGIN_EXT}"
cp "${PLUGIN_PATH}" "${PACKAGE_DIR}/${PLUGIN_NAME_VERSIONED}"
echo "Copied: ${PLUGIN_NAME_VERSIONED}"

# Create symlink
cd "${PACKAGE_DIR}"
ln -sf "${PLUGIN_NAME_VERSIONED}" "codelint-plugin.${PLUGIN_EXT}"
echo "Created symlink: codelint-plugin.${PLUGIN_EXT} -> ${PLUGIN_NAME_VERSIONED}"
cd - > /dev/null

# Create README
cat > "${PACKAGE_DIR}/README.md" << EOF
# codelint plugin (LLVM ${LLVM_VERSION})

Version: ${VERSION}
Platform: ${PLATFORM_NAME}-${ARCH}
LLVM: ${LLVM_VERSION}

## Contents

- \`${PLUGIN_NAME_VERSIONED}\` - codelint clang-tidy plugin
- \`codelint-plugin.${PLUGIN_EXT}\` - Symlink to the plugin

## Requirements

- Ubuntu 22.04 with LLVM ${LLVM_VERSION} installed
- clang-tidy-${LLVM_VERSION} must be available

Install LLVM ${LLVM_VERSION} on Ubuntu 22.04:
\`\`\`bash
apt-get install clang-tidy-${LLVM_VERSION} libclang-${LLVM_VERSION}-dev
\`\`\`

## Usage

\`\`\`bash
# Extract
tar -xzf ${PACKAGE_NAME}.tar.gz
cd ${PACKAGE_NAME}

# Use with clang-tidy
clang-tidy-${LLVM_VERSION} --load=codelint-plugin.${PLUGIN_EXT} --checks='codelint-*' your_file.cpp

# Or using the symlink
clang-tidy-${LLVM_VERSION} --load=codelint-plugin.${PLUGIN_EXT} --checks='codelint-init' src/*.cpp
\`\`\`

## Available Checks

| Check | Auto-fix | Description |
|-------|----------|-------------|
| codelint-init | Yes | Uninitialized variables, dangerous conversions |
| codelint-lint-code | Yes | Style: brace init, unsigned suffix |
| codelint-strict-bool-condition | No | Bool-only conditions |
| codelint-signed-to-unsigned-return | No | POSIX signed→unsigned return |
| codelint-global | No | Global variable detection |
| codelint-global-const-string | No | Global const string optimization |
| codelint-singleton | No | Meyer's Singleton pattern |

## License

MIT License
EOF

# Create tarball
echo ""
echo "Creating tarball..."
mkdir -p "${OUTPUT_DIR}"
cd "${OUTPUT_DIR}"
tar -czvf "${PACKAGE_NAME}.tar.gz" "${PACKAGE_NAME}"
TARBALL_SIZE=$(du -h "${PACKAGE_NAME}.tar.gz" | cut -f1)

echo ""
echo "========================================"
echo "Plugin package created successfully!"
echo "========================================"
echo "Package: ${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz"
echo "Size: ${TARBALL_SIZE}"
echo ""
echo "Contents:"
tar -tzf "${PACKAGE_NAME}.tar.gz"
