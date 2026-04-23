# Clang-Tidy Skill

面向用户的 C++ 静态分析工具，内置 codelint 插件，专注于初始化、类型安全和代码质量检查。

## 核心能力

- 🔧 **自动修复**：`codelint-init` 可自动修复未初始化、风格问题
- 🎯 **场景驱动**：根据用户描述自动匹配检查项
- 📊 **增量分析**：默认只检查修改的文件，快速反馈
- 📋 **分级配置**：default / strict / security 三种预设

## 快速使用

### 场景 1：检查初始化问题（推荐）

```bash
# 自动修复修改的文件
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init --fix

# 查看剩余问题
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init
```

**可自动修复的问题：**

| 问题 | 修复前 | 修复后 |
|-----|--------|--------|
| 未初始化 | `int x;` | `int x{};` |
| 旧语法 | `int x = 5;` | `int x{5};` |
| 缺失后缀 | `unsigned u = 1;` | `unsigned u{1U};` |

### 场景 2：检查 bool 条件

```bash
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-strict-bool-condition
```

**需要手动修复：**

```cpp
// ❌ 错误
if (status) { }
if (strcmp(a, b)) { }  // BUG: strcmp 返回 0 表示相等

// ✅ 正确
if (status == 0) { }
if (strcmp(a, b) == 0) { }
```

### 场景 3：检查 signed→unsigned 转换

```bash
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-signed-to-unsigned-return
```

**危险模式：**

```cpp
// ❌ 危险：read() 返回 -1 表示错误，转 size_t 后变成巨大正数
size_t n = read(fd, buf, count);

// ✅ 正确
ssize_t n = read(fd, buf, count);
if (n < 0) { handle_error(); }
```

### 场景 4：全面代码检查

```bash
# 日常开发（低噪音）
./scripts/run_clang_tidy_diff.py --branch main --preset default --fix

# 生产代码（严格）
./scripts/run_clang_tidy_diff.py --branch main --preset strict --fix

# 安全审计
./scripts/run_clang_tidy_diff.py --branch main --preset security
```

---

## Checks 速查表

| Check | 用户描述 | 自动修复 | 说明 |
|-------|---------|---------|------|
| `codelint-init` | "初始化问题" | ✅ 是 | 未初始化、`=`→`{}`、补后缀 |
| `codelint-strict-bool-condition` | "bool 条件" | ❌ 否 | 强制显式比较 |
| `codelint-signed-to-unsigned-return` | "类型转换" | ❌ 否 | POSIX 函数返回值风险 |
| `codelint-global` | "全局变量" | ❌ 否 | 全局变量检测 |
| `codelint-singleton` | "单例模式" | ❌ 否 | Meyer's Singleton |

---

## 检查范围控制

```bash
# 默认：检查 vs main 分支的修改（推荐）
./scripts/run_clang_tidy_diff.py --branch main --checks=codelint-init

# 检查已暂存的文件
./scripts/run_clang_tidy_diff.py --staged --checks=codelint-init

# 检查最近 3 个 commit
./scripts/run_clang_tidy_diff.py --commits 3 --checks=codelint-init

# 检查整个项目（仅当明确要求时）
clang-tidy --load=lib/codelint-plugin.so --checks=codelinit-init -p build src/**/*.cpp
```

---

## Presets 配置

| Preset | 场景 | 特点 |
|--------|------|------|
| `.clang-tidy.quick-fix` | **快速修复**（推荐） | 专注可自动修复的问题：初始化、现代化、关键 bug |
| `.clang-tidy.bugprone` | **Bug 预防** | 高价值 bug 检测，精选低噪音 checks |
| `.clang-tidy.default` | 日常开发 | 平衡覆盖和噪音 |
| `.clang-tidy.strict` | 生产代码 | 高覆盖，包含 CERT 和 C++ Core Guidelines |
| `.clang-tidy.security` | 安全审计 | 安全专用，关键问题视为 error |

```bash
# 快速修复（最常用）
./scripts/run_clang_tidy_diff.py --branch main --preset quick-fix --fix

# Bug 预防审查
./scripts/run_clang_tidy_diff.py --branch main --preset bugprone

# 使用特定 preset
cp configs/.clang-tidy.default .clang-tidy
./scripts/run_clang_tidy_diff.py --branch main --preset default --fix
```

---

## CI 集成

### GitHub Actions

```yaml
- name: Run clang-tidy
  run: |
    ./scripts/run_clang_tidy_diff.py \
      --branch main \
      --preset default \
      --output sarif > results.sarif

- name: Upload SARIF
  uses: github/codeql-action/upload-sarif@v2
  with:
    sarif_file: results.sarif
```

---

## 环境要求

- Ubuntu 22.04 x86_64
- CMake 3.20+（用于生成 compile_commands.json）
- C++14/17/20 项目

```bash
# 环境准备
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
cmake --preset default
```

---

## License

MIT License
