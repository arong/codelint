# Clang-Tidy Skill

A complete clang-tidy static analysis solution for C++ projects.

## Features

- 📦 **Bundled clang-tidy 21.x binary** - No installation required
- 🔌 **codelint plugin integrated** - Initialization best practices checks
- 📋 **Multiple preset configs** - default, strict, security
- 🔀 **Git diff support** - Incremental PR analysis

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
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH  # Linux only

# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run analysis
clang-tidy --load=lib/codelint-plugin.so -p build src/**/*.cpp
```

## Configuration Presets

### Default (Basic Checks)

```bash
cp share/clang-tidy-skill/configs/.clang-tidy.default .clang-tidy
```

Checks: `bugprone-*`, `modernize-*`, `performance-*`

### Strict (Production Level)

```bash
cp share/clang-tidy-skill/configs/.clang-tidy.strict .clang-tidy
```

Checks: `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `modernize-*`

### Security (Security Audit)

```bash
cp share/clang-tidy-skill/configs/.clang-tidy.security .clang-tidy
```

Checks: `cert-*`, `clang-analyzer-security-*`

## Available Checks

### codelint Plugin Checks

| Check | Description | Auto-fix |
|-------|-------------|----------|
| `codelint-init` | Variable initialization style | ✅ Yes |
| `codelint-strict-bool-condition` | Bool-only conditions | ❌ No |
| `codelint-global` | Global variable detection | ❌ No |
| `codelint-singleton` | Meyer's Singleton pattern | ❌ No |
| `codelint-signed-to-unsigned-return` | Signed→unsigned return | ❌ No |

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
./share/clang-tidy-skill/scripts/run-clang-tidy-diff.py --staged

# Changes vs main branch
./share/clang-tidy-skill/scripts/run-clang-tidy-diff.py --branch main --output sarif
```

## CI Integration

### GitHub Actions

```yaml
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

## Requirements

- CMake 3.20+ (for compile_commands.json generation)
- C++14/17/20 project

## License

MIT License

## Links

- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [codelint Project](https://github.com/user/codelint)
