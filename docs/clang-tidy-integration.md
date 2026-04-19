# Using codelint as a clang-tidy Plugin

## Installation

### Build the plugin

```bash
# Using CMake presets (recommended)
# macOS
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default
# Linux
cmake --preset default
cmake --build build
```

### Install to clang-tidy library directory

```bash
sudo cp build/lib/codelint-plugin.so /usr/local/lib/clang-tidy/
```

## Usage

### Load plugin and run checks

```bash
# Load plugin with specific checks
clang-tidy --load=/usr/local/lib/clang-tidy/codelint-plugin.so \
           --checks='codelint-*' \
           main.cpp

# With compilation database
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-*' \
           -p build \
           src/**/*.cpp

# Apply fixes automatically
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-*' \
           --fix \
           main.cpp
```

### Configuration via .clang-tidy

```yaml
# .clang-tidy
Checks: '-*, codelint-*'
WarningsAsErrors: 'codelint-init'
HeaderFilterRegex: '.*'
```

### Selective checks

```bash
# Only initialization checks
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-init' \
           main.cpp

# Only global/singleton
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-global,codelint-singleton' \
           src/**/*.cpp
```

## Output Formats

clang-tidy provides built-in formats:

```bash
# Default (console)
clang-tidy main.cpp

# YAML
clang-tidy --export-fixes=fixes.yaml main.cpp

# JSON (via clang-tidy-diff)
git diff -U0 HEAD^ | clang-tidy-diff.py \
  --clang-tidy-binary clang-tidy \
  --load codelint-plugin.so

# SARIF (for CI)
clang-tidy --checks='codelint-*' main.cpp | \
  clang-tidy-to-sarif.py > results.sarif
```

## CI Integration

### GitHub Actions

```yaml
# .github/workflows/lint.yml
name: lint
on: [push, pull_request]

jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install clang-tidy
        run: sudo apt install clang-tidy
      - name: Build plugin
        run: |
          cmake -B build
          cmake --build build
      - name: Run codelint
        run: clang-tidy --load=build/lib/codelint-plugin.so \
                        --checks='codelint-*' \
                        -p build \
                        src/**/*.cpp
```

## Available Checks

| Check | Purpose | Auto-fix |
|-------|---------|----------|
| `codelint-init` | Variable initialization style | ✅ Yes |
| `codelint-global` | Global variable detection | ❌ No |
| `codelint-singleton` | Meyer's Singleton pattern | ❌ No |

## Differences from Standalone codelint

| Feature | Standalone | Plugin |
|---------|------------|--------|
| CLI | Custom CLI11 | clang-tidy CLI |
| Git scope | Built-in | ❌ Deleted |
| SARIF output | Built-in | clang-tidy native |
| Fixes | String-based | FixItHint |
| Const suggestions | CFG-based | ❌ Removed |
| Configuration | Custom | .clang-tidy file |
