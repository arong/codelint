#!/bin/bash
# bundle_skill.sh - Bundle clang-tidy skill for distribution
#
# Creates a portable clang-tidy skill package containing:
# - clang-tidy binary (LLVM 21)
# - codelint-plugin.so
# - skill scripts (run_clang_tidy.py, run_clang_tidy_diff.py)
# - skill configs (default, strict, security presets)
# - All required LLVM/Clang shared libraries
#
# Usage: ./bundle_skill.sh [--codelint]
#
# Options:
#   --codelint    Also create codelint package (default: only skill)
#
# Target platforms:
#   - Linux x64
#   - macOS arm64 (Apple Silicon)
#   - macOS x64 (Intel)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
SKILL_DIR="${PROJECT_ROOT}/.skill/clang-tidy"
OUTPUT_DIR="${PROJECT_ROOT}/package-output"
LLVM_VERSION="21"

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
else
    PLATFORM_NAME="linux"
fi

SKILL_PACKAGE_NAME="clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}"
SKILL_PACKAGE_DIR="${OUTPUT_DIR}/${SKILL_PACKAGE_NAME}"

CODELINT_PACKAGE_NAME="codelint-${VERSION}-${PLATFORM_NAME}-${ARCH}"
CODELINT_PACKAGE_DIR="${OUTPUT_DIR}/${CODELINT_PACKAGE_NAME}"

echo "========================================"
echo "Creating clang-tidy skill package"
echo "========================================"
echo "Version: ${VERSION}"
echo "Platform: ${PLATFORM_NAME}-${ARCH}"
echo "Skill directory: ${SKILL_DIR}"
echo "Output: ${SKILL_PACKAGE_DIR}"
echo ""

find_clang_tidy() {
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        candidates=(
            "/opt/homebrew/opt/llvm@${LLVM_VERSION}/bin/clang-tidy"
            "/usr/local/opt/llvm@${LLVM_VERSION}/bin/clang-tidy"
        )
    else
        candidates=(
            "/usr/bin/clang-tidy-${LLVM_VERSION}"
            "/usr/lib/llvm-${LLVM_VERSION}/bin/clang-tidy"
            "/usr/bin/clang-tidy"
        )
    fi
    
    for candidate in "${candidates[@]}"; do
        if [ -f "$candidate" ]; then
            echo "$candidate"
            return
        fi
    done
    
    echo ""
}

find_llvm_lib_dir() {
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        candidates=(
            "/opt/homebrew/opt/llvm@${LLVM_VERSION}/lib"
            "/usr/local/opt/llvm@${LLVM_VERSION}/lib"
        )
    else
        candidates=(
            "/usr/lib/llvm-${LLVM_VERSION}/lib"
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

copy_lib() {
    local lib_path="$1"
    local dest_dir="$2"
    local lib_name=$(basename "$lib_path")
    
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        local skip_patterns="libc.dylib libm.dylib libdl.dylib libpthread.dylib libSystem.dylib"
    else
        local skip_patterns="libc.so libm.so libdl.so libpthread.so librt.so ld-linux"
    fi
    
    for pattern in $skip_patterns; do
        if [[ "$lib_name" == "$pattern"* ]]; then
            return
        fi
    done
    
    if [ ! -f "${dest_dir}/${lib_name}" ]; then
        cp -L "$lib_path" "${dest_dir}/${lib_name}" 2>/dev/null || true
        echo "    Copied: $lib_name"
    fi
}

collect_dependencies() {
    local binary="$1"
    local dest_dir="$2"
    
    echo "  Analyzing dependencies of $(basename $binary)..."
    
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        otool -L "$binary" | grep -v "^\t/usr/lib/" | grep -v "^\t/System/" | awk '{print $1}' | while read lib; do
            if [ -f "$lib" ]; then
                copy_lib "$lib" "$dest_dir"
            fi
        done
    else
        ldd "$binary" 2>/dev/null | grep "=> /" | awk '{print $3}' | while read lib; do
            copy_lib "$lib" "$dest_dir"
        done
    fi
}

create_skill_package() {
    echo "Creating skill package..."
    echo ""
    
    rm -rf "${SKILL_PACKAGE_DIR}"
    mkdir -p "${SKILL_PACKAGE_DIR}/bin"
    mkdir -p "${SKILL_PACKAGE_DIR}/lib"
    mkdir -p "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/scripts"
    mkdir -p "${SKILL_PACKAGE_DIR}/share/clang-tidy-skill/configs"
    
    CLANG_TIDY=$(find_clang_tidy)
    if [ -z "$CLANG_TIDY" ]; then
        echo "ERROR: clang-tidy not found!"
        echo "Install: brew install llvm@${LLVM_VERSION} (macOS) or apt install clang-tidy-${LLVM_VERSION} (Linux)"
        exit 1
    fi
    echo "[1/7] clang-tidy: $CLANG_TIDY"
    
    LLVM_LIB_DIR=$(find_llvm_lib_dir)
    LLVM_SHARE_DIR=$(find_llvm_share_dir)
    
    PLUGIN=$(find_plugin)
    if [ -z "$PLUGIN" ]; then
        echo "ERROR: codelint plugin not found!"
        echo "Build: cmake --build build"
        exit 1
    fi
    echo "[2/7] plugin: $PLUGIN"
    
    echo "[3/7] Copying binaries..."
    cp "$CLANG_TIDY" "${SKILL_PACKAGE_DIR}/bin/clang-tidy"
    chmod +x "${SKILL_PACKAGE_DIR}/bin/clang-tidy"
    
    if [ -f "${LLVM_SHARE_DIR}/clang-tidy-diff.py" ]; then
        cp "${LLVM_SHARE_DIR}/clang-tidy-diff.py" "${SKILL_PACKAGE_DIR}/bin/"
        chmod +x "${SKILL_PACKAGE_DIR}/bin/clang-tidy-diff.py"
        echo "    Copied: clang-tidy-diff.py"
    fi
    
    if [ -f "${LLVM_SHARE_DIR}/run-clang-tidy.py" ]; then
        cp "${LLVM_SHARE_DIR}/run-clang-tidy.py" "${SKILL_PACKAGE_DIR}/bin/"
        chmod +x "${SKILL_PACKAGE_DIR}/bin/run-clang-tidy.py"
        echo "    Copied: run-clang-tidy.py"
    fi
    
    echo "[4/7] Copying plugin..."
    cp "$PLUGIN" "${SKILL_PACKAGE_DIR}/lib/"
    PLUGIN_NAME=$(basename "$PLUGIN")
    
    echo "[5/7] Copying skill scripts..."
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
    
    echo "[6/7] Copying skill configs..."
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
    
    echo "[7/7] Collecting library dependencies..."
    collect_dependencies "${SKILL_PACKAGE_DIR}/bin/clang-tidy" "${SKILL_PACKAGE_DIR}/lib"
    collect_dependencies "$PLUGIN" "${SKILL_PACKAGE_DIR}/lib"
    
    if [ "$PLATFORM_NAME" = "darwin" ]; then
        for lib in "libclang-cpp.${LLVM_VERSION}.dylib" "libLLVM.${LLVM_VERSION}.dylib"; do
            if [ -f "${LLVM_LIB_DIR}/${lib}" ] && [ ! -f "${SKILL_PACKAGE_DIR}/lib/${lib}" ]; then
                cp "${LLVM_LIB_DIR}/${lib}" "${SKILL_PACKAGE_DIR}/lib/"
                echo "    Copied: $lib"
            fi
        done
    else
        for lib in "libclang-cpp.so.${LLVM_VERSION}" "libLLVM-${LLVM_VERSION}.so"; do
            if [ -f "${LLVM_LIB_DIR}/${lib}" ] && [ ! -f "${SKILL_PACKAGE_DIR}/lib/${lib}" ]; then
                cp "${LLVM_LIB_DIR}/${lib}" "${SKILL_PACKAGE_DIR}/lib/"
                echo "    Copied: $lib"
            fi
        done
        
        cd "${SKILL_PACKAGE_DIR}/lib"
        [ -f "libclang-cpp.so.${LLVM_VERSION}" ] && ln -sf libclang-cpp.so.${LLVM_VERSION} libclang-cpp.so
        [ -f "libLLVM-${LLVM_VERSION}.so" ] && ln -sf libLLVM-${LLVM_VERSION}.so libLLVM.so
        cd - > /dev/null
    fi
    
    create_skill_readme "${SKILL_PACKAGE_DIR}"
    
    if [ "$PLATFORM_NAME" = "linux" ] && command -v patchelf >/dev/null 2>&1; then
        echo "Setting RPATH..."
        patchelf --set-rpath '$ORIGIN/../lib' "${SKILL_PACKAGE_DIR}/bin/clang-tidy" 2>/dev/null || true
    fi
    
    echo ""
    echo "Creating skill tarball..."
    cd "${OUTPUT_DIR}"
    tar -czvf "${SKILL_PACKAGE_NAME}.tar.gz" "${SKILL_PACKAGE_NAME}"
    TARBALL_SIZE=$(du -h "${SKILL_PACKAGE_NAME}.tar.gz" | cut -f1)
    
    echo ""
    echo "========================================"
    echo "Skill package created!"
    echo "========================================"
    echo "Package: ${OUTPUT_DIR}/${SKILL_PACKAGE_NAME}.tar.gz"
    echo "Size: ${TARBALL_SIZE}"
}

create_codelint_package() {
    echo ""
    echo "Creating codelint package..."
    echo ""
    
    rm -rf "${CODELINT_PACKAGE_DIR}"
    mkdir -p "${CODELINT_PACKAGE_DIR}/bin"
    mkdir -p "${CODELINT_PACKAGE_DIR}/lib"
    
    CLANG_TIDY=$(find_clang_tidy)
    PLUGIN=$(find_plugin)
    
    if [ -z "$CLANG_TIDY" ] || [ -z "$PLUGIN" ]; then
        echo "ERROR: Required binaries not found"
        exit 1
    fi
    
    cp "$CLANG_TIDY" "${CODELINT_PACKAGE_DIR}/bin/"
    cp "$PLUGIN" "${CODELINT_PACKAGE_DIR}/lib/"
    
    if [ -f "${PROJECT_ROOT}/bin/codelint" ]; then
        cp "${PROJECT_ROOT}/bin/codelint" "${CODELINT_PACKAGE_DIR}/bin/"
        chmod +x "${CODELINT_PACKAGE_DIR}/bin/codelint"
    fi
    
    if [ -f "${PROJECT_ROOT}/bin/codelint-diff" ]; then
        cp "${PROJECT_ROOT}/bin/codelint-diff" "${CODELINT_PACKAGE_DIR}/bin/"
        chmod +x "${CODELINT_PACKAGE_DIR}/bin/codelint-diff"
    fi
    
    LLVM_SHARE_DIR=$(find_llvm_share_dir)
    if [ -f "${LLVM_SHARE_DIR}/clang-tidy-diff.py" ]; then
        cp "${LLVM_SHARE_DIR}/clang-tidy-diff.py" "${CODELINT_PACKAGE_DIR}/bin/"
    fi
    
    collect_dependencies "${CODELINT_PACKAGE_DIR}/bin/clang-tidy" "${CODELINT_PACKAGE_DIR}/lib"
    collect_dependencies "$PLUGIN" "${CODELINT_PACKAGE_DIR}/lib"
    
    create_codelint_readme "${CODELINT_PACKAGE_DIR}"
    
    if [ "$PLATFORM_NAME" = "linux" ] && command -v patchelf >/dev/null 2>&1; then
        patchelf --set-rpath '$ORIGIN/../lib' "${CODELINT_PACKAGE_DIR}/bin/clang-tidy" 2>/dev/null || true
    fi
    
    echo ""
    echo "Creating codelint tarball..."
    cd "${OUTPUT_DIR}"
    tar -czvf "${CODELINT_PACKAGE_NAME}.tar.gz" "${CODELINT_PACKAGE_NAME}"
    TARBALL_SIZE=$(du -h "${CODELINT_PACKAGE_NAME}.tar.gz" | cut -f1)
    
    echo ""
    echo "========================================"
    echo "Codelint package created!"
    echo "========================================"
    echo "Package: ${OUTPUT_DIR}/${CODELINT_PACKAGE_NAME}.tar.gz"
    echo "Size: ${TARBALL_SIZE}"
}

create_skill_readme() {
    local pkg_dir="$1"
    cat > "${pkg_dir}/README.md" << EOF
# clang-tidy-skill - Static C++ Analysis Package

Version: ${VERSION}
Platform: ${PLATFORM_NAME}-${ARCH}
LLVM: ${LLVM_VERSION}

## Contents

- \`bin/clang-tidy\` - clang-tidy binary (LLVM ${LLVM_VERSION})
- \`bin/clang-tidy-diff.py\` - Git diff scanner
- \`bin/run-clang-tidy.py\` - Parallel execution wrapper
- \`lib/codelint-plugin.*\` - codelint clang-tidy plugin
- \`share/clang-tidy-skill/scripts/\` - Skill runner scripts
- \`share/clang-tidy-skill/configs/\` - Preset configurations

## Quick Start

\`\`\`bash
# Extract
tar -xzf clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}.tar.gz

# Set environment
export PATH=\$PWD/clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}/bin:\$PATH
$(if [ "$PLATFORM_NAME" = "linux" ]; then echo "export LD_LIBRARY_PATH=\$PWD/clang-tidy-skill-${VERSION}-${PLATFORM_NAME}-${ARCH}/lib:\$LD_LIBRARY_PATH"; fi)

# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run analysis
clang-tidy --load=lib/codelint-plugin.so --checks='codelint-*' -p build src/*.cpp
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
| \`default\` | Basic checks (bugprone, modernize) |
| \`strict\` | Production level (CERT, C++ Core Guidelines) |
| \`security\` | Security-focused (cert, security-analyzer) |

Copy preset to project root:
\`\`\`bash
cp share/clang-tidy-skill/configs/.clang-tidy.default .clang-tidy
\`\`\`

## Available Checks

| Check | Auto-fix | Description |
|-------|----------|-------------|
| codelint-init | Yes | Variable initialization |
| codelint-strict-bool-condition | No | Bool-only conditions |
| codelint-global | No | Global variables |
| codelint-singleton | No | Meyer's Singleton |
| bugprone-* | Partial | Bug detection |
| modernize-* | Yes | Modern C++ |

## Requirements

$(if [ "$PLATFORM_NAME" = "darwin" ]; then echo "- macOS $(if [ "$ARCH" = "arm64" ]; then echo "12.0+ (Apple Silicon)"; else echo "10.15+ (Intel)"; fi)"; else echo "- Ubuntu 22.04 or compatible Linux"; fi)
- CMake 3.20+ (for compile_commands.json)
- C++14/17/20 project

## License

MIT License
EOF
}

create_codelint_readme() {
    local pkg_dir="$1"
    cat > "${pkg_dir}/README.md" << EOF
# codelint - clang-tidy Plugin Package

Version: ${VERSION}
Platform: ${PLATFORM_NAME}-${ARCH}
LLVM: ${LLVM_VERSION}

## Contents

- \`bin/clang-tidy\` - clang-tidy binary
- \`bin/codelint\` - Wrapper script (auto-loads plugin)
- \`bin/codelint-diff\` - Incremental scanner
- \`lib/codelint-plugin.*\` - codelint plugin

## Usage

\`\`\`bash
# Using wrapper (recommended)
./bin/codelint --fix src/main.cpp

# With compile_commands.json
./bin/codelint -p build src/*.cpp

# Incremental scan
git diff -U0 HEAD^ | ./bin/codelint-diff -p1
\`\`\`

## Available Checks

| Check | Auto-fix | Description |
|-------|----------|-------------|
| codelint-init | Yes | Variable initialization style |
| codelinit-strict-bool-condition | No | Bool-only conditions |
| codelinit-global | No | Global variable detection |
| codelinit-singleton | No | Meyer's Singleton pattern |

## Requirements

$(if [ "$PLATFORM_NAME" = "darwin" ]; then echo "- macOS"; else echo "- Ubuntu 22.04+"; fi)
- CMake 3.20+
- compile_commands.json

## License

MIT License
EOF
}

main() {
    INCLUDE_CODELINT=false
    if [ "$1" = "--codelint" ]; then
        INCLUDE_CODELINT=true
    fi
    
    mkdir -p "${OUTPUT_DIR}"
    
    create_skill_package
    
    if [ "$INCLUDE_CODELINT" = true ]; then
        create_codelint_package
    fi
    
    echo ""
    echo "All packages created in: ${OUTPUT_DIR}"
    ls -la "${OUTPUT_DIR}"/*.tar.gz 2>/dev/null || true
}

main "$@"