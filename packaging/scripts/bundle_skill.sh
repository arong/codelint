#!/bin/bash
# bundle_skill.sh - Bundle clang-tidy skill for distribution
#
# Creates a minimal skill package containing:
# - codelint-plugin.so (for LLVM 15)
# - skill scripts (run_clang_tidy.py, run_clang_tidy_diff.py)
# - skill configs (default, strict, security presets)
#
# Target environment is assumed to have LLVM 15 (including clang-tidy-15) installed.
# No LLVM libraries or clang-tidy binary are bundled.
#
# Usage: ./bundle_skill.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
SKILL_DIR="${PROJECT_ROOT}/skills/clang-tidy"
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

SKILL_PACKAGE_NAME="clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}"
SKILL_PACKAGE_DIR="${OUTPUT_DIR}/${SKILL_PACKAGE_NAME}"

echo "========================================"
echo "Creating clang-tidy skill package"
echo "========================================"
echo "Version: ${VERSION}"
echo "Platform: ${PLATFORM_NAME}-${ARCH}"
echo "Skill directory: ${SKILL_DIR}"
echo "Output: ${SKILL_PACKAGE_DIR}"
echo ""

find_plugin() {
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        plugin="${BUILD_DIR}/lib/codelint-plugin.dylib"
    else
        plugin="${BUILD_DIR}/lib/codelint-plugin.so"
    fi

    if [ -f "$plugin" ]; then
        echo "$plugin"
        return
    fi

    echo ""
}

find_llvm_share_dir() {
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        candidates=(
            "/opt/homebrew/opt/llvm@${LLVM_VERSION}/share/clang"
            "/usr/local/opt/llvm@${LLVM_VERSION}/share/clang"
        )
    else
        candidates=(
            "/usr/lib/llvm-${LLVM_VERSION}/share/clang"
            "/usr/share/clang"
        )
    fi

    for candidate in "${candidates[@]}"; do
        if [ -d "$candidate" ]; then
            echo "$candidate"
            return
        fi
    done

    echo ""
}

echo "Creating skill package..."
echo ""

rm -rf "${SKILL_PACKAGE_DIR}"
mkdir -p "${SKILL_PACKAGE_DIR}/lib"
mkdir -p "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/scripts"
mkdir -p "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/configs"

PLUGIN=$(find_plugin)
if [ -z "$PLUGIN" ]; then
    echo "ERROR: codelint plugin not found!"
    echo "Build: cmake --build build"
    exit 1
fi
echo "[1/4] plugin: $PLUGIN"

LLVM_SHARE_DIR=$(find_llvm_share_dir)

echo "[2/4] Copying plugin..."
cp "$PLUGIN" "${SKILL_PACKAGE_DIR}/lib/"
PLUGIN_NAME=$(basename "$PLUGIN")
echo "    Copied: $PLUGIN_NAME"

echo "[3/4] Copying skill scripts..."
if [ -d "${SKILL_DIR}/scripts" ] && [ "$(ls -A ${SKILL_DIR}/scripts 2>/dev/null)" ]; then
    cp -r "${SKILL_DIR}/scripts/"* "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/scripts/"
    chmod +x "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/scripts/"*
    echo "    Copied scripts:"
    ls "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/scripts/" | while read f; do
        echo "      $f"
    done
else
    echo "    WARNING: No skill scripts found"
fi

echo "[4/4] Copying skill configs..."
if [ -d "${SKILL_DIR}/configs" ] && [ "$(ls -A ${SKILL_DIR}/configs 2>/dev/null)" ]; then
    shopt -s dotglob
    cp -r "${SKILL_DIR}/configs/"* "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/configs/"
    shopt -u dotglob
    echo "    Copied configs:"
    ls "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/configs/" | while read f; do
        echo "      $f"
    done
else
    echo "    WARNING: No skill configs found"
fi

create_skill_readme "${SKILL_PACKAGE_DIR}"

echo ""
echo "Creating skill tarball..."
mkdir -p "${OUTPUT_DIR}"
cd "${OUTPUT_DIR}"
tar -czvf "${SKILL_PACKAGE_NAME}.tar.gz" "${SKILL_PACKAGE_NAME}"
TARBALL_SIZE=$(du -h "${SKILL_PACKAGE_NAME}.tar.gz" | cut -f1)

echo ""
echo "========================================"
echo "Skill package created!"
echo "========================================"
echo "Package: ${OUTPUT_DIR}/${SKILL_PACKAGE_NAME}.tar.gz"
echo "Size: ${TARBALL_SIZE}"
echo ""
echo "Contents:"
tar -tzf "${SKILL_PACKAGE_NAME}.tar.gz"

create_skill_readme() {
    local pkg_dir="$1"
    cat > "${pkg_dir}/README.md" << EOF
# clang-tidy-skill - Static C++ Analysis Package

Version: ${VERSION}
Platform: ${PLATFORM_NAME}-${ARCH}
LLVM: ${LLVM_VERSION} (system-installed)

## Contents

- \`lib/codelint-plugin.${PLUGIN_EXT}\` - codelint clang-tidy plugin
- \`share/clang-tidy-skill/scripts/\` - Skill runner scripts
- \`share/clang-tidy-skill/configs/\` - Preset configurations

## Requirements

- Ubuntu 22.04 with LLVM ${LLVM_VERSION} installed
- clang-tidy-${LLVM_VERSION} must be available

Install LLVM ${LLVM_VERSION} on Ubuntu 22.04:
\`\`\`bash
apt-get install clang-tidy-${LLVM_VERSION} libclang-${LLVM_VERSION}-dev
\`\`\`

## Quick Start

\`\`\`bash
# Extract
tar -xzf clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}.tar.gz
cd clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}

# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run analysis using system clang-tidy
clang-tidy-${LLVM_VERSION} --load=lib/codelint-plugin.${PLUGIN_EXT} --checks='codelint-*' -p build src/*.cpp
\`\`\`

## Skill Scripts

\`\`\`bash
# Run with preset configuration
./share/clang-tidy-skill/scripts/run_clang_tidy.py -p build --preset strict

# Git diff scan for PR review
./share/clang-tidy-skill/scripts/run_clang_tidy_diff.py --branch main --output sarif
\`\`\`

## Configuration Presets

| Preset | Description |
|--------|-------------|
| \`codelint\` | codelint plugin only (initialization/safety) |
| \`default\` | Basic checks (bugprone, modernize) |
| \`strict\` | Production level (CERT, C++ Core Guidelines) |
| \`security\` | Security-focused (cert, security-analyzer) |

Copy preset to project root:
\`\`\`bash
cp share/clang-tidy-skill/configs/.clang-tidy.codelint .clang-tidy
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
}
