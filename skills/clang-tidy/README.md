# Clang-Tidy Skill

A complete clang-tidy static analysis solution for C++ projects.

## Features

- 📦 **Bundled clang-tidy 21.x binary** - No installation required
- 🔌 **codelint plugin integrated** - 7 initialization and safety checks
- 📋 **Multiple preset configs** - codelint, default, strict, security
- 🔀 **Git diff support** - Incremental PR analysis
- 🖥️ **Cross-platform** - macOS (arm64/x64), Linux (x64)

## Quick Start

### 1. Download

Download the appropriate package for your platform:

| Platform | Package |
|----------|---------|
| macOS (Apple Silicon) | `clang-tidy-skill-0.1.0-darwin-arm64.tar.gz` |
| macOS (Intel) | `clang-tidy-skill-0.1.0-darwin-x64.tar.gz` |
| Linux (x64) | `clang-tidy-skill-0.1.0-linux-x64.tar.gz` |

### 2. Extract

```bash
tar -xzf clang-tidy-skill-*.tar.gz
```

### 3. Run

```bash
# Set environment
export PATH=$PWD/bin:$PATH
# macOS
export DYLD_LIBRARY_PATH=$PWD/lib:$DYLD_LIBRARY_PATH
# Linux
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run analysis
clang-tidy --load=lib/codelint-plugin.so -p build src/**/*.cpp
```

## Configuration Presets

### Codelint (Plugin Only)

```bash
cp <SKILL_PATH>/configs/.clang-tidy.codelint .clang-tidy
```

Checks: `codelint-*` only - focused initialization and safety audit

### Default (Basic Checks)

```bash
cp <SKILL_PATH>/configs/.clang-tidy.default .clang-tidy
```

Checks: `bugprone-*`, `modernize-*`, `performance-*`

### Strict (Production Level)

```bash
cp <SKILL_PATH>/configs/.clang-tidy.strict .clang-tidy
```

Checks: `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `modernize-*`

### Security (Security Audit)

```bash
cp <SKILL_PATH>/configs/.clang-tidy.security .clang-tidy
```

Checks: `cert-*`, `clang-analyzer-security-*`

## Available Checks

### codelint Plugin Checks (7 total)

| Check | Description | Auto-fix |
|-------|-------------|----------|
| `codelint-init` | Uninitialized variables, dangerous conversions (int→bool, narrowing) | ✅ Partial |
| `codelint-lint-code` | Style: `=` → `{}`, unsigned suffix `U`/`UL` | ✅ Yes |
| `codelint-strict-bool-condition` | Bool-only conditions (if/while/for) | ❌ No |
| `codelint-signed-to-unsigned-return` | POSIX signed return → unsigned (e.g., `read()`) | ❌ No |
| `codelint-global` | Global variable detection | ❌ No |
| `codelint-global-const-string` | Global const string → `constexpr const char*` | ❌ No |
| `codelint-singleton` | Meyer's Singleton pattern detection | ❌ No |

⚠️ **Note**: `codelint-strict-bool-condition` does not support C++23

### Clang-Tidy Built-in

| Category | Purpose |
|----------|---------|
| `bugprone-*` | Bug detection |
| `modernize-*` | Modern C++ idioms |
| `performance-*` | Performance optimization |
| `readability-*` | Code readability |
| `cert-*` | CERT C++ secure coding |
| `cppcoreguidelines-*` | C++ Core Guidelines |

## Git Diff Analysis

Analyze only changed files for PR review:

```bash
# Staged changes
<SKILL_PATH>/scripts/run_clang_tidy_diff.py --staged

# Changes vs main branch
<SKILL_PATH>/scripts/run_clang_tidy_diff.py --branch main --output sarif

# Recent commits
<SKILL_PATH>/scripts/run_clang_tidy_diff.py --commits 5
```

## Requirements

- CMake 3.20+ (for compile_commands.json generation)
- C++14/17/20 project (C++23 not fully supported)

## License

MIT License

## Links

- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [codelint Project](https://github.com/user/codelint)
