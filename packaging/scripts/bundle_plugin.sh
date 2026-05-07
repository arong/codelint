#!/bin/bash
# bundle_plugin.sh - Bundle codelint plugin for distribution
#
# Creates a minimal package containing only the codelint plugin.
# Target environment is assumed to have LLVM (including clang-tidy) installed.
#
# Usage: ./bundle_plugin.sh
#
# Output: codelint-core-<distro_id><distro_version>-LLVM<llvm_version>.tar.gz
#   e.g. codelint-core-Ubuntu22.04-LLVM15.tar.gz
#
# Environment variables:
#   LLVM_VERSION - LLVM major version (default: 15)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
OUTPUT_DIR="${PROJECT_ROOT}/package-output"
LLVM_VERSION="${LLVM_VERSION:-15}"

get_version() {
    if [ -n "$GITHUB_REF" ] && [[ "$GITHUB_REF" == refs/tags/v* ]]; then
        echo "${GITHUB_REF#refs/tags/v}"
    elif git rev-parse --git-dir >/dev/null 2>&1; then
        git describe --tags --always 2>/dev/null || echo "dev"
    else
        echo "dev"
    fi
}

get_distro_id() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "${ID}" | sed 's/.*/\u&/'
    elif command -v lsb_release >/dev/null 2>&1; then
        lsb_release -si
    else
        echo "Linux"
    fi
}

get_distro_version() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "${VERSION_ID}"
    elif command -v lsb_release >/dev/null 2>&1; then
        lsb_release -rs
    else
        echo "unknown"
    fi
}

VERSION=$(get_version)
DISTRO_ID=$(get_distro_id)
DISTRO_VERSION=$(get_distro_version)
ARCH=$(uname -m)
PLATFORM=$(uname -s)

if [ "$PLATFORM" = "Darwin" ]; then
    PLUGIN_EXT="dylib"
else
    PLUGIN_EXT="so"
fi

PACKAGE_NAME="codelint-core-${DISTRO_ID}${DISTRO_VERSION}-LLVM${LLVM_VERSION}"
PACKAGE_DIR="${OUTPUT_DIR}/${PACKAGE_NAME}"

echo "========================================"
echo "Creating codelint-core package"
echo "========================================"
echo "Version: ${VERSION}"
echo "Distro: ${DISTRO_ID} ${DISTRO_VERSION}"
echo "LLVM: ${LLVM_VERSION}"
echo "Output: ${PACKAGE_DIR}"
echo ""

# Clean and create output directory
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

# Find the plugin
PLUGIN_PATH="${BUILD_DIR}/lib/codelint-core.${PLUGIN_EXT}"
if [ ! -f "${PLUGIN_PATH}" ]; then
    echo "ERROR: codelint-core.${PLUGIN_EXT} not found at ${PLUGIN_PATH}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

# Copy plugin
cp "${PLUGIN_PATH}" "${PACKAGE_DIR}/codelint-core.${PLUGIN_EXT}"
echo "Copied: codelint-core.${PLUGIN_EXT}"

# Create README
cat > "${PACKAGE_DIR}/README.md" << EOF
# codelint-core

Version: ${VERSION}
Platform: ${DISTRO_ID} ${DISTRO_VERSION}
LLVM: ${LLVM_VERSION}

## Contents

- \`codelint-core.${PLUGIN_EXT}\` - codelint clang-tidy plugin

## Requirements

- ${DISTRO_ID} ${DISTRO_VERSION} with LLVM ${LLVM_VERSION} installed
- clang-tidy-${LLVM_VERSION} must be available

Install LLVM ${LLVM_VERSION} on ${DISTRO_ID} ${DISTRO_VERSION}:
\`\`\`bash
apt-get install clang-tidy-${LLVM_VERSION} libclang-${LLVM_VERSION}-dev
\`\`\`

## Usage

\`\`\`bash
# Extract
tar -xzf ${PACKAGE_NAME}.tar.gz
cd ${PACKAGE_NAME}

# Use with clang-tidy
clang-tidy-${LLVM_VERSION} --load=codelint-core.${PLUGIN_EXT} --checks='codelint-*' your_file.cpp
\`\`\`

## Available Checks

| Check | Auto-fix | Description |
|-------|----------|-------------|
| codelint-init | Yes | Uninitialized variables, dangerous conversions |
| codelint-lint-code | Yes | Style: brace init, unsigned suffix |
| codelint-strict-bool-condition | No | Bool-only conditions |
| codelint-signed-to-unsigned-return | No | POSIX signed→unsigned return |
| codelint-global-const-string | No | Global const string optimization |

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
echo "Package created successfully!"
echo "========================================"
echo "Package: ${OUTPUT_DIR}/${PACKAGE_NAME}.tar.gz"
echo "Size: ${TARBALL_SIZE}"
echo ""
echo "Contents:"
tar -tzf "${PACKAGE_NAME}.tar.gz"
