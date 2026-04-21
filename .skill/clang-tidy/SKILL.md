---
name: clang-tidy
description: Static C++ code analysis with clang-tidy + codelint plugin. Bundled binary for out-of-box experience.
---

# Clang-Tidy Skill

## Overview

This skill provides a complete clang-tidy static analysis solution:

- **Bundled clang-tidy 21.x binary** - No installation required
- **codelint plugin integrated** - Initialization best practices checks
- **Multiple preset configs** - default, strict, security
- **Git diff support** - Incremental PR analysis

## When to Use

Use this skill when:

- Analyzing C++ code for bugs and modernization opportunities
- PR review - incremental analysis of changed files
- New project setup - generate `.clang-tidy` configuration
- Code quality gate in CI/CD

**When NOT to use:**

- Non-C++ projects (no applicable checks)
- Projects without `compile_commands.json` (analysis will be inaccurate)
- Quick typo fixes (clang-tidy is too heavy for trivial changes)

## Workflow

### Step 1: Prepare Environment

Before running clang-tidy, ensure:

```bash
# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Or use CMake preset
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default
cmake --build build
```

**Alternative**: Use `-p build` flag to specify compile commands directory.

### Step 2: Choose Configuration

Select preset based on project needs:

| Preset | Checks | Noise Level | Use Case |
|--------|--------|-------------|----------|
| `default` | bugprone, modernize, performance | Low | New projects |
| `strict` | + cert, cppcoreguidelines | Medium | Production code |
| `security` | cert, security-analyzer | Focused | Security audit |

Copy preset to project:

```bash
# Use default preset
cp .skill/clang-tidy/configs/.clang-tidy.default .clang-tidy

# Or use strict preset
cp .skill/clang-tidy/configs/.clang-tidy.strict .clang-tidy
```

### Step 3: Run Analysis

**Analyze entire project:**

```bash
# Using bundled run-clang-tidy.py (parallel execution)
run-clang-tidy.py -p build -j $(nproc)

# Or direct clang-tidy invocation
clang-tidy --load=lib/codelint-plugin.so \
           --checks='codelint-*' \
           -p build \
           src/**/*.cpp
```

**Analyze specific files:**

```bash
clang-tidy --load=lib/codelint-plugin.so \
           --checks='codelint-*' \
           --fix \
           src/main.cpp
```

**Git diff scan (PR review):**

```bash
# Analyze staged changes
run-clang-tidy-diff.py --staged

# Analyze changes vs main branch
run-clang-tidy-diff.py --branch main --output sarif

# Analyze recent commits
run-clang-tidy-diff.py --commits 5
```

### Step 4: Apply Fixes

Auto-fix available for certain checks:

| Check | Auto-fix | Fix Description |
|-------|----------|-----------------|
| `codelint-init` | ✅ | Convert to brace initialization |
| `modernize-loop-convert` | ✅ | Convert to range-based for |
| `modernize-use-nullptr` | ✅ | Replace NULL with nullptr |
| `performance-*` | Partial | Performance optimizations |
| `codelint-strict-bool-condition` | ❌ | Manual fix required |
| `codelint-global` | ❌ | Manual fix required |

Apply fixes:

```bash
clang-tidy --load=lib/codelint-plugin.so \
           --checks='codelinit-*' \
           --fix \
           --format-style=file \
           src/**/*.cpp
```

### Step 5: Review Results

Clang-tidy output formats:

| Format | Flag | Use Case |
|--------|------|----------|
| Console (default) | None | Local development |
| YAML | `--export-fixes=fixes.yaml` | Fix tracking |
| SARIF | `run-clang-tidy-diff.py --output sarif` | GitHub Actions |

## Integration

### CMake Integration

Add to `CMakeLists.txt`:

```cmake
# Enable clang-tidy checks during build
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_CLANG_TIDY
      clang-tidy
      --load=${CMAKE_SOURCE_DIR}/lib/codelint-plugin.so
      --checks=-*,codelint-*
  )
endif()
```

### GitHub Actions CI

```yaml
name: clang-tidy
on: [push, pull_request]

jobs:
  clang-tidy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build

      - name: Generate compile commands
        run: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

      - name: Download clang-tidy-skill
        run: |
          wget https://github.com/user/codelint/releases/download/v0.1.0/clang-tidy-skill-0.1.0-linux-x64.tar.gz
          tar -xzf clang-tidy-skill-*.tar.gz
          export PATH=$GITHUB_WORKSPACE/bin:$PATH
          export LD_LIBRARY_PATH=$GITHUB_WORKSPACE/lib:$LD_LIBRARY_PATH

      - name: Run clang-tidy
        run: |
          ./share/clang-tidy-skill/scripts/run-clang-tidy-diff.py \
            --branch main \
            --output sarif \
            > results.sarif

      - name: Upload SARIF
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: results.sarif
```

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit
FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|cc|h|hpp)$')
if [ -n "$FILES" ]; then
  clang-tidy --load=lib/codelint-plugin.so \
             --checks=-*,codelinit-* \
             --fix \
             -p build \
             $FILES
  git add $FILES
fi
```

## Available Checks

### codelint Plugin Checks

| Check | Category | Auto-fix | Description |
|-------|----------|----------|-------------|
| `codelint-init` | Initialization | ✅ Yes | Variable initialization style |
| `codelint-strict-bool-condition` | Logic | ❌ No | Bool-only conditions |
| `codelint-global` | Architecture | ❌ No | Global variable detection |
| `codelint-singleton` | Architecture | ❌ No | Meyer's Singleton pattern |
| `codelint-signed-to-unsigned-return` | Safety | ❌ No | Signed→unsigned return detection |

### Clang-Tidy Built-in Checks

| Category | Examples | Auto-fix |
|----------|----------|----------|
| `bugprone-*` | argument-comment, dangling-handle | Partial |
| `modernize-*` | loop-convert, use-auto, use-nullptr | ✅ Yes |
| `performance-*` | for-range-copy, move-const-arg | Partial |
| `readability-*` | braces-around-statements | ✅ Yes |
| `cert-*` | err34-c (scanf check) | Partial |
| `cppcoreguidelines-*` | avoid-do-while | ❌ No |

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| `compile_commands.json not found` | Missing CMake configuration | Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| `clang-tidy not found` | Bundled binary not in PATH | Use `clang-tidy-wrapper.sh` which sets environment |
| `library load error` | Missing LLVM libraries | Ensure `lib/` directory contains all libraries |
| `codelint-plugin not found` | Plugin not bundled | Check `lib/codelint-plugin.so` exists |
| `SDK path error (macOS)` | Missing macOS SDK | Run `SDKROOT=$(xcrun --show-sdk-path) cmake --preset default` |

### Platform-specific Notes

**macOS:**

```bash
# Set SDK path for compile_commands.json
SDKROOT=$(xcrun --show-sdk-path) cmake -B build

# Use dylib version of plugin
clang-tidy --load=lib/codelint-plugin.dylib
```

**Linux:**

```bash
# Set LD_LIBRARY_PATH for bundled libraries
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

# Use .so version of plugin
clang-tidy --load=lib/codelint-plugin.so
```

## Verification Checklist

After applying clang-tidy fixes:

- [ ] All auto-fixable warnings resolved
- [ ] Manual fixes for non-auto-fixable warnings
- [ ] Tests pass after changes
- [ ] No new warnings introduced
- [ ] Code formatted with clang-format (if using `--format-style=file`)