---
name: clang-tidy
description: Static C++ code analysis with bundled clang-tidy + codelint plugin. Out-of-box experience for C++ developers.
---

# Clang-Tidy Skill

## What This Skill Does

Run clang-tidy static analysis on your C++ code with:
- **Bundled clang-tidy 21.x** - No installation needed
- **codelint plugin** - Custom initialization checks
- **Ready-to-use presets** - default, strict, security configs

## Quick Start

```bash
# 1. Generate compile_commands.json (required)
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Copy a preset config to your project
cp share/clang-tidy-skill/configs/.clang-tidy.default .clang-tidy

# 3. Run analysis
clang-tidy -p build src/**/*.cpp
```

## When to Use

| Scenario | Recommended |
|----------|-------------|
| PR review (changed files only) | `run-clang-tidy-diff.py --branch main` |
| Full project scan | `run-clang-tidy.py -p build -j $(nproc)` |
| Single file fix | `clang-tidy --fix src/main.cpp` |
| CI/CD integration | `run-clang-tidy-diff.py --output sarif` |

**Prerequisites:**
- C++ project with `compile_commands.json`
- CMake 3.20+ to generate compile commands

## Usage Examples

### Analyze Changed Files (PR Review)

```bash
# Staged changes only
run-clang-tidy-diff.py --staged

# Changes vs target branch
run-clang-tidy-diff.py --branch main --output sarif
```

### Analyze Full Project

```bash
# Parallel analysis (recommended)
run-clang-tidy.py -p build -j $(nproc)

# Single file with auto-fix
clang-tidy --fix -p build src/main.cpp
```

## Configuration Presets

| Preset | Checks | Use Case |
|--------|--------|----------|
| `.clang-tidy.default` | bugprone, modernize, performance | General development |
| `.clang-tidy.strict` | + cert, cppcoreguidelines | Production code |
| `.clang-tidy.security` | cert, security-analyzer | Security audit |

```bash
cp share/clang-tidy-skill/configs/.clang-tidy.default .clang-tidy
```

## CI Integration (GitHub Actions)

```yaml
- name: Run clang-tidy
  run: |
    ./share/clang-tidy-skill/scripts/run-clang-tidy-diff.py \
      --branch main --output sarif > results.sarif

- name: Upload SARIF
  uses: github/codeql-action/upload-sarif@v2
  with:
    sarif_file: results.sarif
```

## Available Checks

### codelint Plugin (Custom)

| Check | Auto-fix | Description |
|-------|----------|-------------|
| `codelint-init` | ✅ | Initialization style |
| `codelint-strict-bool-condition` | ❌ | Bool-only conditions |
| `codelint-global` | ❌ | Global variable detection |
| `codelint-singleton` | ❌ | Singleton pattern |

### Built-in clang-tidy

Key categories: `bugprone-*`, `modernize-*`, `performance-*`, `readability-*`, `cert-*`

## Common Issues

| Issue | Solution |
|-------|----------|
| `compile_commands.json not found` | Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| `clang-tidy not found` | Ensure `bin/` is in PATH |
| `SDK path error (macOS)` | Run `SDKROOT=$(xcrun --show-sdk-path) cmake -B build` |
| `LD_LIBRARY_PATH error (Linux)` | Set `export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH` |


