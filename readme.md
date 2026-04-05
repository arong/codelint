# Codelint - C++ Code Analysis Tool

Codelint 是一个基于 LLVM LibTooling 的 C++ 代码静态分析工具，用于检查代码中的常见问题并提供自动修复建议。

## 功能特性

### 1. `check_init` - 变量初始化检查

检查变量初始化风格，推荐现代 C++ 最佳实践：

#### 检测项

| 问题类型 | 说明 | 示例修复 |
|---------|------|---------|
| **未初始化变量** | 变量声明时未显式初始化 | `int c;` → `int c{};` |
| **`=` 初始化转 `{}`** | 建议使用统一初始化语法 | `int a = 5;` → `int a{5};` |
| **无符号整型后缀** | unsigned 类型建议添加 `U` 后缀 | `unsigned int b = 42;` → `unsigned int b{42U};` |
| **const 建议** | 变量未被修改，建议添加 `const` | `int x{42};` → `const int x{42};` |
| **constexpr 建议** | 编译期常量，建议使用 `constexpr` | `const int x{42};` → `constexpr int x{42};` |

#### 智能跳过规则

以下情况会自动跳过，避免误报：

- **auto 声明** - 类型推导需要保留 `=` 语法
- **for 循环变量** - `for (int i = 0; ...)` 等循环变量
- **union 成员** - union 成员初始化有特殊语义
- **enum class 无零值** - 枚举类型没有零值成员时跳过
- **extern 声明** - 外部链接声明不修改
- **异常变量** - `catch (const auto& e)` 中的异常变量

```bash
./codelint check_init <file> [--fix]
```

### 2. `find_global` - 全局变量检测

查找项目中的所有全局变量，帮助识别潜在的状态管理问题：

```bash
./codelint find_global <file_or_directory>
```

**输出内容**：
- 变量名称、类型
- 所在文件、行号、列号
- 初始化值（如有）

### 3. `find_singleton` - 单例模式检测

检测代码中的 Meyer's Singleton 实现（返回静态局部变量引用的函数）：

```bash
./codelint find_singleton <file_or_directory>
```

**检测模式**：
```cpp
// 会检测到的模式
static T& instance() {
    static T inst{...};  // Meyer's Singleton
    return inst;
}
```

## 输出格式

### 文本输出（默认）

```
/path/to/file.cpp:10:5: warning: Variable should use '{}' syntax for initialization [init]
  int a = 5;
      ^
  suggestion: int a{5};
```

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
