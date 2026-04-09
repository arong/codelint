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

### Deleted Features

The following features were removed in the clang-tidy plugin migration:
- ❌ Git scope filtering (`--scope modified`)
- ❌ Const/constexpr suggestions (requires CFG analysis)
- ❌ Custom output formats (use clang-tidy's native formats)
- ❌ Custom CLI (use clang-tidy's CLI)

## Running Tests

```bash
cmake --build build
cd build
ctest --output-on-failure
```

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

### JSON 输出

添加 `--output-json` 标志以 JSON 格式输出，方便与 CI/CD 或其他工具集成：

```bash
./codelint --output-json check_init tests/test.cpp
./codelint --output-json find_global src/
./codelint --output-json find_singleton src/
```

**JSON 格式示例**：
```json
{
  "issues": [
    {
      "type": "INIT_EQUALS_SYNTAX",
      "severity": "warning",
      "checker": "init",
      "name": "a",
      "type_str": "int",
      "file": "/path/to/file.cpp",
      "line": 10,
      "column": 5,
      "description": "Variable should use '{}' syntax for initialization",
      "suggestion": "int a{5};",
      "fixable": true
    }
  ]
}
```

## 回归测试

项目包含完整的回归测试套件，确保功能稳定性：

```bash
bash tests/run_regression_tests.sh
```

## 快速开始

### 环境要求

**macOS (Homebrew)**:
```bash
# 安装 LLVM 和 libgit2
brew install llvm@21 libgit2
```

**Ubuntu/Debian**:
```bash
# 安装 LLVM 和 libgit2 开发库
sudo apt install llvm-dev libclang-dev clang libgit2-dev
```

**Arch Linux**:
```bash
sudo pacman -S llvm libs git
```

### 构建项目

**macOS**:
```bash
# 创建构建目录并配置（必须指定 LLVM_DIR）
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm

# 构建项目
cmake --build build -j$(sysctl -n hw.ncpu)
```

**Linux**:
```bash
# 创建构建目录并配置
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 构建项目
cmake --build build -j$(nproc)
```

**注意**：macOS 上必须通过 `-DLLVM_DIR` 指定 LLVM 配置路径，否则 CMake 无法找到 Homebrew 安装的 LLVM。

### 运行测试

```bash
cd build
ctest --output-on-failure
```

### 使用示例

```bash
# 检查单个文件
./codelint check_init src/main.cpp

# 检查并自动修复
./codelint check_init src/main.cpp --fix

# 检查整个目录
./codelint check_init src/

# 检查全局变量
./codelint find_global src/

# 检查单例模式
./codelint find_singleton src/

# JSON 输出，用于 CI 集成
./codelint --output-json check_init src/
```

## 增量分析 (--scope)

只检查修改的代码，而不是整个代码库：

```bash
# 检查未提交的更改（工作目录）
codelint check_init src/ --scope modified

# 检查已暂存的更改（git add 但未提交）
codelint find_global src/ --scope staged

# 检查特定提交
codelint find_singleton src/ --scope commit:HEAD

# 检查 PR 与 main 的差异
codelint check_init src/ --scope pr:main

# 检查两个分支之间的差异
codelint find_global src/ --scope diff:main...feature
```

**功能：**

- **文件级过滤**：只编译和检查修改过的文件（更快）
- **行级过滤**：只在修改的行上报告问题（更精确）
- **支持所有检查器**：check_init, find_global, find_singleton

## 技术架构

- **基于 LLVM LibTooling** - 使用 Clang AST 进行精确的语法分析
- **AST Visitor 模式** - 使用 `RecursiveASTVisitor` 遍历语法树
- **CFG 数据流分析** - 使用控制流图分析变量的生命周期和修改情况
- **模块化设计** - 检查器可独立运行或组合使用

## 打包发布

项目支持打包为 AppImage 格式，便于分发和部署：

```bash
# 构建项目后执行打包脚本
python3 packaging/scripts/create_appimage.py
```

打包完成后会在项目根目录生成 `codelint-VERSION-ARCH.AppImage` 文件。

详细打包说明请参考 [packaging/README.md](packaging/README.md)。

## 许可证

MIT License
Test
