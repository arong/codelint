# Codelint - clang-tidy Plugin for C++ Code Analysis

Codelint is now a **clang-tidy plugin** that provides custom checks for C++ code analysis. It was refactored from a standalone LibTooling binary to integrate seamlessly with clang-tidy's ecosystem.

## Features

### Three Custom Checks

| Check | Purpose | Auto-fix |
|-------|---------|----------|
| **codelint-init** | Variable initialization style (uninitialized, `=` → `{}`, unsigned suffix, macro skip, C-array) | ✅ Yes |
| **codelint-global** | Global variable detection | ❌ No |
| **codelint-singleton** | Meyer's Singleton pattern detection | ❌ No |

### codelint-init Features

1. **Uninitialized variables** - Detects variables without explicit initialization
2. **Equals syntax** - Suggests brace initialization `int x{5}` instead of `int x = 5`
3. **Unsigned suffix** - Adds `U` suffix to unsigned integer literals
4. **Macro skip** - Automatically skips variables defined inside macros
5. **C-style arrays** - Provides specific warning for uninitialized C-style arrays

## Installation

### Prerequisites

- **LLVM/Clang 21+** (with clang-tidy)
- **CMake 3.20+**
- **C++20 compiler**

### Build the Plugin

**macOS (Homebrew):**
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm

cmake --build build -j$(sysctl -n hw.ncpu)
```

**Linux:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Install

```bash
sudo cp build/lib/codelint-plugin.so /usr/local/lib/clang-tidy/
```

## Usage

### Basic Usage

```bash
# Run all codelint checks
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

### Configuration

Create a `.clang-tidy` file in your project root:

```yaml
Checks: '-*, codelint-*'
WarningsAsErrors: 'codelint-init'
HeaderFilterRegex: '.*'
```

### Selective Checks

```bash
# Only initialization checks
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-init' \
           main.cpp

# Only global/singleton checks
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-global,codelint-singleton' \
           src/**/*.cpp
```

## Output Formats

clang-tidy supports multiple output formats:

```bash
# Default console output
clang-tidy main.cpp

# YAML export
clang-tidy --export-fixes=fixes.yaml main.cpp

# SARIF for CI/CD
clang-tidy --checks='codelint-*' main.cpp | \
  clang-tidy-to-sarif.py > results.sarif
```

## CI Integration

### GitHub Actions

```yaml
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
        run: cmake -B build && cmake --build build
      - name: Run codelint
        run: clang-tidy --load=build/lib/codelint-plugin.so \
                        --checks='codelint-*' \
                        -p build \
                        src/**/*.cpp
```

## Documentation

- **[codelint-init](docs/check-docs/codelint-init.md)** - Variable initialization checks
- **[codelint-global](docs/check-docs/codelint-global.md)** - Global variable detection
- **[codelint-singleton](docs/check-docs/codelint-singleton.md)** - Singleton pattern detection
- **[clang-tidy Integration Guide](docs/clang-tidy-integration.md)** - Detailed usage instructions

## Migration from Standalone

If you were using the old standalone codelint binary:

| Old Command | New Command |
|-------------|-------------|
| `codelint check_init src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-init' src/**/*.cpp` |
| `codelint find_global src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-global' src/**/*.cpp` |
| `codelint find_singleton src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-singleton' src/**/*.cpp` |

---

## Documentation

- **[codelint-init](docs/check-docs/codelint-init.md)** - Variable initialization checks
- **[codelint-global](docs/check-docs/codelint-global.md)** - Global variable detection
- **[codelint-singleton](docs/check-docs/codelint-singleton.md)** - Singleton pattern detection
- **[clang-tidy Integration Guide](docs/clang-tidy-integration.md)** - Detailed usage instructions

---

## Migration from Standalone

If you were using the old standalone codelint binary:

| Old Command | New Command |
|-------------|-------------|
| `codelint check_init src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-init' src/**/*.cpp` |
| `codelint find_global src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-global' src/**/*.cpp` |
| `codelint find_singleton src/` | `clang-tidy --load=codelint-plugin.so --checks='codelint-singleton' src/**/*.cpp` |

### Deleted Features

The following features were removed in the clang-tidy plugin migration:
- ❌ Git scope filtering (`--scope modified`)
- ❌ Const/constexpr suggestions (requires CFG analysis)
- ❌ Custom output formats (use clang-tidy's native formats)
- ❌ Custom CLI (use clang-tidy's CLI)

## Packaging

Project supports packaging as AppImage format for easy distribution:

```bash
# After building the project
python3 packaging/scripts/create_appimage.py
```

This creates `codelint-VERSION-ARCH.AppImage` in the project root.

See [packaging/README.md](packaging/README.md) for detailed packaging instructions.

## Architecture

```
codelint-plugin.so
├── CodelintModule.cpp          # Plugin registration
└── checks/
    ├── InitCheck.cpp           # Initialization checks
    ├── GlobalCheck.cpp         # Global variable detection
    └── SingletonCheck.cpp      # Singleton pattern detection
```

## License

MIT License
Test
