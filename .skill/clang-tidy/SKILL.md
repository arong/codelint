---
name: clang-tidy
description: Static C++ code analysis with bundled clang-tidy + codelint plugin for Linux x86_64.
---

# Clang-Tidy Skill

## 使用范式：场景驱动检查

当用户要求"用 clang-tidy 检查..."时，按以下流程执行：

### Step 1: 识别用户意图 → 匹配 Checks

根据用户描述的关键词，选择对应的 checks：

| 用户描述关键词 | 推荐 Checks | 自动修复 | 说明 |
|--------------|------------|---------|------|
| "初始化"、"未初始化"、"init" | `codelint-init` | ✅ 是 | 变量 `{}` 初始化、`=`→`{}`、补全 `U`/`UL` 后缀 |
| "bool 条件"、"条件判断"、"if 条件" | `codelint-strict-bool-condition` | ❌ 否 | 强制显式比较，如 `if (status == 0)` |
| "signed 转 unsigned"、"返回值类型" | `codelint-signed-to-unsigned-return` | ❌ 否 | 检测 `read()`/`open()` 等 POSIX 函数返回值的类型转换风险 |
| "全局变量"、"global" | `codelint-global` | ❌ 否 | 检测全局变量使用 |
| "单例"、"singleton" | `codelint-singleton` | ❌ 否 | 检测 Meyer's Singleton 模式 |
| "全面检查"、"代码质量"、"general" | preset: `default` 或 `strict` | 部分 | 使用预设配置进行全量检查 |
| "安全"、"security"、"漏洞" | preset: `security` | 部分 | 安全审计专用配置 |

**渐进展开策略**：
- 如果用户描述模糊（如"检查代码问题"），先问具体关注点，或推荐 `codelint-init`（最常用且可自动修复）
- 如果用户说"所有问题"，使用 `default` preset

### Step 2: 确定检查范围 → 优先增量检查

**默认优先检查用户修改的文件**，而非全量扫描：

```bash
# 检查当前工作区 vs main 分支的修改（最常用）
./scripts/run_clang_tidy_diff.py --branch main --checks=<CHECKS>

# 检查已暂存（staged）的文件
./scripts/run_clang_tidy_diff.py --staged --checks=<CHECKS>

# 检查最近 N 个 commit
./scripts/run_clang_tidy_diff.py --commits 3 --checks=<CHECKS>
```

**仅当用户明确要求"检查整个项目"或"所有文件"时才全量扫描：**

```bash
# 全量检查（不推荐作为默认）
clang-tidy --load=lib/codelint-plugin.so --checks=<CHECKS> -p build src/**/*.cpp
```

### Step 3: 执行并修复

**总是先尝试自动修复，再报告剩余问题：**

```bash
# 1. 尝试自动修复修改的文件
./scripts/run_clang_tidy_diff.py --branch main --checks=<CHECKS> --fix

# 2. 再次检查，确认是否还有未修复的问题
./scripts/run_clang_tidy_diff.py --branch main --checks=<CHECKS>
```

**对于无法自动修复的问题，给出具体的代码修改建议。**

---

## 典型场景示例

### 场景 A：用户说"检查一下初始化问题"

```bash
# 执行：检查修改的文件，尝试自动修复
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init --fix

# 再次检查剩余问题
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init
```

**codelint-init 可修复的内容：**
| 问题 | 修复前 | 修复后 |
|-----|--------|--------|
| 未初始化 | `int x;` | `int x{};` |
| 等于语法 | `int x = 5;` | `int x{5};` |
| 缺失 U 后缀 | `unsigned u = 1;` | `unsigned u{1U};` |
| 缺失 UL 后缀 | `uint64_t n = 42;` | `uint64_t n{42UL};` |

### 场景 B：用户说"检查一下 bool 条件"

```bash
# 执行：检查修改的文件（此 check 不支持自动修复）
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-strict-bool-condition
```

**需要手动修复的示例：**
```cpp
// ❌ 错误 - 需要修复
if (status) { }
if (strcmp(a, b)) { }

// ✅ 正确
if (status == 0) { }
if (strcmp(a, b) == 0) { }
```

### 场景 C：用户说"全面检查一下代码质量"

```bash
# 使用 default preset，检查修改的文件
./scripts/run_clang_tidy_diff.py --branch main --preset default --fix

# 或更严格的 strict preset
./scripts/run_clang_tidy_diff.py --branch main --preset strict --fix
```

---

## Checks 详细参考

### codelint-init（✅ 自动修复）

- **未初始化变量**：`int x;` → `int x{};`
- **等于转 Brace**：`int x = 5;` → `int x{5};`
- **补全 U 后缀**：`unsigned u = 1;` → `unsigned u{1U};`
- **补全 UL 后缀**：`uint64_t n = 42;` → `uint64_t n{42UL};`

### codelint-strict-bool-condition（❌ 手动修复）

- 检测：`if (integer)`、`while (pointer)`、`if (strcmp(a,b))`
- 要求：显式比较，如 `if (x == 0)`、`if (ptr != nullptr)`
- 原因：`strcmp` 返回 0 表示相等，`if (strcmp)` 逻辑反了

### codelint-signed-to-unsigned-return（❌ 手动修复）

- 检测：`size_t n = read(fd, buf, count);`
- 原因：`read()` 返回 `-1` 表示错误，转 `size_t` 后变成巨大正数
- 修复：使用 `ssize_t n = read(...); if (n < 0) handle_error();`

### codelint-global（❌ 手动修复）

- 检测全局变量定义
- 建议：改用单例模式或依赖注入

### codelint-singleton（❌ 手动修复）

- 检测 Meyer's Singleton 模式
- 用于代码审查时的模式识别

---

## Presets 参考

| Preset | 适用场景 | 包含 Checks |
|--------|---------|------------|
| `default` | 日常开发，低噪音 | bugprone-*, modernize-*, performance-*, readability-* |
| `strict` | 生产代码，高覆盖 | + cert-*, cppcoreguidelines-*, clang-analyzer-* |
| `security` | 安全审计 | cert-*, clang-analyzer-security.*, bugprone-security |

使用方式：
```bash
# 使用 preset 检查修改的文件
./scripts/run_clang_tidy_diff.py --branch main --preset default --fix
```

---

## 内置 Checks 离线参考

> 部署环境可能无法连接外网查询 clang-tidy 文档，以下提供常用内置 checks 的离线决策参考。

### bugprone-*（Bug 检测）

| Check | 说明 | 自动修复 | 常见场景 |
|-------|------|---------|---------|
| `bugprone-argument-comment` | 参数注释与实际参数不匹配 | 部分 | 函数参数较多时 |
| `bugprone-dangling-handle` | 返回临时对象的引用/指针 | ❌ | string_view 等 |
| `bugprone-sizeof-container` | `sizeof(vector)` 误用 | ✅ | 容器大小计算 |
| `bugprone-suspicious-memset-usage` | memset 参数错误 | ❌ | 内存操作 |
| `bugprone-use-after-move` | 移动后继续使用变量 | ❌ | 移动语义 |
| `bugprone-string-constructor` | string 构造函数误用 | ❌ | 字符串处理 |

**推荐场景**：所有项目都应启用，高价值低噪音。

### modernize-*（现代化 C++）

| Check | 说明 | 自动修复 | 常见场景 |
|-------|------|---------|---------|
| `modernize-use-nullptr` | `NULL` → `nullptr` | ✅ | C++11 迁移 |
| `modernize-use-auto` | 显式类型 → `auto` | ✅ | 类型冗长时 |
| `modernize-loop-convert` | for → range-based for | ✅ | 遍历容器 |
| `modernize-use-override` | 虚函数加 `override` | ✅ | 继承体系 |
| `modernize-use-default-member-init` | 构造函数初始化 → 类内初始化 | ✅ | 构造函数简化 |
| `modernize-use-equals-default` | 空函数体 → `= default` | ✅ | 特殊成员函数 |

**推荐场景**：希望代码现代化时使用，大部分可自动修复。

### performance-*（性能优化）

| Check | 说明 | 自动修复 | 常见场景 |
|-------|------|---------|---------|
| `performance-for-range-copy` | range-for 中不必要的拷贝 | ✅ | 遍历容器 |
| `performance-move-const-arg` | 对 const 对象使用 move | ✅ | 移动语义误用 |
| `performance-unnecessary-value-param` | 值传递可改为 const 引用 | 部分 | 函数参数优化 |
| `performance-noexcept-move-constructor` | move 构造函数缺少 noexcept | ❌ | 异常安全 |

**推荐场景**：性能敏感代码，或作为代码审查补充。

### readability-*（可读性）

| Check | 说明 | 自动修复 | 噪音程度 |
|-------|------|---------|---------|
| `readability-braces-around-statements` | if/for 缺少大括号 | ✅ | 低 |
| `readability-named-parameter` | 未命名参数 | ✅ | 低 |
| `readability-magic-numbers` | 魔法数字 | ❌ | **高** |
| `readability-function-cognitive-complexity` | 函数复杂度过高 | ❌ | **高** |
| `readability-identifier-length` | 标识符过短 | ❌ | **高** |

**推荐场景**：default preset 中已排除高噪音 checks（magic-numbers, cognitive-complexity, identifier-length）。如需启用，单独指定。

### cert-*（CERT C++ 安全编码）

| Check | 说明 | 自动修复 |
|-------|------|---------|
| `cert-dcl03-c` | 静态/动态 cast 使用 C 风格 | ✅ |
| `cert-dcl16-c` | unsigned 字面量缺少后缀 | ✅ |
| `cert-err34-c` | `atoi`/`atol` 等不安全的字符串转数字 | ❌ |
| `cert-oop57-cpp` | 虚函数调用风险 | ❌ |

**推荐场景**：安全审计、生产代码，strict / security preset 已包含。

### cppcoreguidelines-*（C++ 核心指南）

| Check | 说明 | 自动修复 | 噪音 |
|-------|------|---------|------|
| `cppcoreguidelines-pro-type-member-init` | 类成员未初始化 | 部分 | 中 |
| `cppcoreguidelines-special-member-functions` | 缺少拷贝/移动构造函数 | ❌ | 中 |
| `cppcoreguidelines-owning-memory` | 原始指针管理资源 | ❌ | 中 |
| `cppcoreguidelines-avoid-magic-numbers` | 魔法数字（同 readability） | ❌ | **高** |
| `cppcoreguidelines-pro-bounds-*` | 数组越界/指针运算 | ❌ | 中 |

**推荐场景**：strict preset 包含，但排除了多个高噪音 checks。

### clang-analyzer-*（深度静态分析）

| Check | 说明 | 误报率 |
|-------|------|--------|
| `clang-analyzer-core.*` | 核心分析（空指针、未初始化等） | 低 |
| `clang-analyzer-cplusplus.*` | C++ 专用分析 | 低 |
| `clang-analyzer-security.*` | 安全漏洞分析（缓冲区溢出等） | 低 |
| `clang-analyzer-alpha.*` | 实验性分析 | **高** |

**推荐场景**：strict preset 包含（排除 alpha.*），security preset 重点使用 security.*。

---

## 环境要求

```bash
# 运行前确保环境变量已设置
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

# 确保 compile_commands.json 已生成
cmake --preset default
```

如果分析失败：
1. 检查 `compile_commands.json` 是否存在（`cmake --preset default` 生成）
2. 检查 `LD_LIBRARY_PATH` 是否包含 `lib/` 目录
3. 检查文件是否在 git 追踪范围内（增量检查需要）

---

## 输出格式

```bash
# 控制台输出（默认）
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init

# SARIF 格式（CI 集成）
./scripts/run_clang_tidy_diff.py --branch main --output sarif > results.sarif

# JSON 格式（程序解析）
./scripts/run_clang_tidy_diff.py --branch main --output json
```
