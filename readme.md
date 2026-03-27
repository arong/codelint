# CNDY - C++ Code Analysis Tool

CNDY 是一个 C++ 代码分析工具，用于检查代码中的常见问题并自动修复。

## 功能特性

### 1. `find_global` - 查找全局变量
查找项目中的所有全局变量并输出其位置、类型和初始化值。

```bash
./cndy find_global <file_or_directory>
```

### 2. `find_singleton` - 查找单例模式
检测代码中的 Meyer's Singleton 实现。

```bash
./cndy find_singleton <file_or_directory>
```

### 3. `check_init` - 检查变量初始化
检查变量初始化相关问题：
- **未初始化变量**: `int c;` → `int c{};`
- **`=` 初始化转 `{}`**: `int a = 5;` → `int a{5};`
- **无符号整型后缀**: `unsigned int b = 42;` → `unsigned int b{42U};`

```bash
./cndy check_init <file> [--fix]
```

### 4. `lint` - 综合代码检查
整合多个检查器进行全面的代码质量检查。

```bash
./cndy lint <file_or_directory> [options]
```

**lint 选项：**
- `--only=<checker>` - 只运行指定的检查器 (init, const, global, singleton)
- `--exclude=<checker>` - 排除指定的检查器
- `--fix` - 自动修复发现的问题
- `--severity=<level>` - 按严重程度过滤 (error, warning, info)
- `--output-json` - 以 JSON 格式输出结果

## 输出格式

### 文本输出
包含关键信息：变量/函数/类的名称、类型、所在行、文件的绝对路径。

### JSON 输出
添加 `--output-json` 全局标志，以 JSON 格式输出，方便与其他工具集成：

```bash
./cndy --output-json check_init tests/test.cpp
./cndy --output-json lint src/
```

## 回归测试

项目包含完整的回归测试套件，确保功能不回退：

```bash
# 运行所有回归测试
bash tests/run_regression_tests.sh

# 运行特定测试
bash tests/test_init_check_regression.sh
```

## 快速开始

```bash
# 构建项目
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# 运行检查
./cndy check_init ../tests/test_check_init.cpp --fix
./cndy lint ../src/

# 运行回归测试
cd ..
bash tests/run_regression_tests.sh
```

## 许可证

MIT License
