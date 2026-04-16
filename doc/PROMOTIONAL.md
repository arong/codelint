# Codelint — Modern C++ 初始化规范自动化工具

> **基于 LLVM/Clang 的静态代码分析插件，让危险代码无处藏身**

---

## 目录

1. [产品概述](#一产品概述)
2. [当前功能](#二当前功能)
3. [技术架构](#三技术架构)
4. [未来规划](#四未来规划)
5. [快速上手](#五快速上手)

---

## 一、产品概述

### 1.1 定位

**Codelint** 是一款基于 LLVM/Clang 的静态代码分析插件，专注于 C++ 初始化最佳实践的自动化检测与修复。

它不是又一个"只报告问题"的 lint 工具 —— Codelint 的核心价值在于**自动修复**，让开发者从繁琐的代码风格整改中解放出来，专注于业务逻辑。

### 1.2 核心价值

| 价值主张 | 说明 |
|----------|------|
| **零容忍未定义行为** | 自动捕获未初始化变量、危险隐式转换等可能导致崩溃的问题 |
| **自动修复，而非仅仅报告** | 80%+ 检测到的问题可一键修复，无需手动修改 |
| **AI 友好设计** | 专为 AI 辅助开发场景优化，无缝集成到 AI 工作流 |
| **本地运行** | 无数据外泄风险，企业级安全合规 |

### 1.3 技术背书

- **基础架构**: LLVM 21 + Clang-tidy 插件体系
- **分析精度**: AST 级语义分析，非文本匹配
- **语言标准**: C++20
- **设计哲学**: 模块化、可扩展、零误报

### 1.4 解决的问题

```cpp
// ❌ 危险代码：未初始化变量可能导致未定义行为
int count;
process(count);  // count 的值是什么？

// ✅ Codelint 自动修复后
int count{};  // 明确初始化为 0
process(count);
```

```cpp
// ❌ 危险代码：隐式 narrowing conversion
int value = 3.14;  // 3.14 被截断为 3

// ✅ Codelint 修复后（使用 brace init 阻止 narrowing）
int value{3};  // 编译期警告 narrowing
```

```cpp
// ❌ 危险代码：隐式类型转换
bool success = Init();  // 从 int 隐式转换到 bool

// ✅ Codelint 报告 Error（需手动修复）
// Error: assigning integer to bool is dangerous
bool success{true};  // 开发者显式决定如何修复
```

---

## 二、当前功能

### 2.1 核心检查器：codelint-init

**codelint-init** 是 Codelint 的核心检查器，专注于统一 C++ 变量初始化风格，强制推广 Modern C++ 的 Brace Initialization (`{}`) 最佳实践。

#### 为什么选择 Brace Init？

| 特性 | 传统 `=` 初始化 | Brace Init `{}` |
|------|----------------|-----------------|
| Narrowing 检查 | ❌ 允许，运行时截断 | ✅ 编译期阻止 |
| 统一语法 | ❌ 不同类型不同语法 | ✅ 所有类型统一 |
| 防止最 vexing parse | ❌ `A a();` 被解析为函数 | ✅ `A a{};` 明确为对象 |
| 默认初始化 | ❌ 内置类型不初始化 | ✅ `{}` 零初始化 |

#### 自动修复能力

| 问题类型 | 修复前 | 修复后 | 修复方式 |
|----------|--------|--------|----------|
| **未初始化变量** | `int x;` | `int x{}` | 自动 |
| **等号初始化** | `int x = 5;` | `int x{5};` | 自动 |
| **字符串初始化** | `std::string s = "hi";` | `std::string s{"hi"};` | 自动 |
| **缺少 U 后缀** | `unsigned u = 100;` | `unsigned u{100U};` | 自动 |
| **危险布尔转换** | `bool ret = Init();` | ❌ Error 警告 | 手动 |

#### 智能跳过机制（零误报）

Codelint 内置完整的跳过列表，自动识别以下场景并**不误报**：

| 场景 | 示例 | 跳过原因 |
|------|------|----------|
| **For 循环** | `for (int i = 0; i < n; i++)` | C 语言习惯用法 |
| **Catch 参数** | `catch (const Exception& e)` | 异常参数由 catch 初始化 |
| **宏定义** | `#define MACRO() { int x = 1; }` | 宏内无法安全修改 |
| **Auto 类型** | `auto x = getValue()` | 类型推导需要初始化表达式 |
| **Extern 声明** | `extern int x;` | 定义在其他翻译单元 |
| **Union 成员** | `union { int a; float b; }` | Union 特殊语义 |
| **Bitfields** | `int flags : 4;` | 位域特殊语法 |
| **引用** | `int& ref = x;` | 引用必须绑定到已有对象 |
| **默认参数** | `void foo(int a = 10)` | 函数签名的一部分 |
| **Lambda 捕获** | `[](int x) { ... }` | Lambda 参数上下文 |
| **已 Brace 初始化** | `int x = {};` | 已经是 brace init |
| **Narrowing** | `int x = 3.14;` | 仅警告，不修改（避免改变行为） |
| **类型拓宽** | `float f = 5;` | 安全的隐式转换 |

#### initializer_list 构造函数处理

Codelint 智能识别含 `std::initializer_list` 构造函数的类，避免错误转换：

```cpp
class MyArray {
public:
  MyArray(std::initializer_list<int> list);  // initializer_list 构造函数
  MyArray(int value);                         // 普通构造函数
};

MyArray arr = 1;   // ❌ 跳过 - 改为{1}会调用 initializer_list 构造函数
MyArray arr2{10};  // ✅ 正确 - 显式 brace init

std::string s = "hello";  // ✅ 修复为 std::string s{"hello"}
// std::string 有 initializer_list<char>，但"hello"是 const char*，不是 initializer_list
```

#### 类成员初始化检查

Codelint 检测构造函数中未初始化的成员变量：

```cpp
class Widget {
  int id;
  std::string name;

  Widget() {}           // ❌ 警告: 'id', 'name' 未初始化

  Widget() : id{}, name{} {}  // ✅ 正确
};
```

---

## 三、技术架构

### 3.1 架构概览

```
┌─────────────────────────────────────┐
│         clang-tidy 主程序          │
│         (用户直接调用)              │
├─────────────────────────────────────┤
│     CodelintModule (插件入口)       │
│     ┌─────────────────────────┐    │
│     │ addCheckFactories()     │    │
│     │ - registerCheck<InitCheck> │ │
│     └─────────────────────────┘    │
├─────────────────────────────────────┤
│           InitCheck                 │
│     (核心业务逻辑实现)              │
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│      Clang AST Matchers            │
│      (语义级代码分析引擎)          │
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│         FixItHint API              │
│      (自动修复代码生成)            │
└─────────────────────────────────────┘
```

### 3.2 工作流程

```
用户源代码 (.cpp/.h)
       │
       ▼
┌─────────────────────────────────────┐
│  Clang 解析器 → AST (抽象语法树)    │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  AST Matchers 模式匹配              │
│  varDecl(hasNoInit(), ...)          │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  InitCheck::check()                 │
│  - 分析节点                         │
│  - 生成诊断                         │
│  - 提供 FixItHint                   │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  终端输出 / --fix 应用修复          │
└─────────────────────────────────────┘
```

### 3.3 核心组件

#### CodelintModule (插件入口)

```cpp
// src/codelint_plugin/CodelintModule.cpp
class CodelintModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
    CheckFactories.registerCheck<InitCheck>("codelint-init");
  }
};
```

#### InitCheck (核心检查器)

```cpp
// src/codelint_plugin/checks/InitCheck.cpp
void InitCheck::registerMatchers(MatchFinder* Finder) {
  // 匹配未初始化变量
  Finder->addMatcher(
    varDecl(unless(parmVarDecl()),
            unless(hasType(autoType())),
            unless(hasAncestor(forStmt())),
            ...)
        .bind("uninit"),
    this);
}

void InitCheck::check(const MatchFinder::MatchResult& Result) {
  if (const auto* VD = Result.Nodes.getNodeAs<VarDecl>("uninit")) {
    // 生成诊断 + 修复
    diag(VD->getLocation(), "variable is not explicitly initialized")
        << FixItHint::CreateInsertion(VD->getEndLoc(), "{}");
  }
}
```

### 3.4 可扩展性设计

Codelint 的架构设计支持快速扩展新的检查规则。添加新检查器只需 4 步：

#### 步骤 1: 创建头文件

```cpp
// include/codelint/checks/MyNewCheck.h
#pragma once
#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::codelint {

class MyNewCheck : public ClangTidyCheck {
public:
  MyNewCheck(StringRef Name, ClangTidyContext* Context);
  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace clang::tidy::codelint
```

#### 步骤 2: 实现检查逻辑

```cpp
// src/codelint_plugin/checks/MyNewCheck.cpp
#include "codelint/checks/MyNewCheck.h"

using namespace clang::ast_matchers;

void MyNewCheck::registerMatchers(MatchFinder* Finder) {
  Finder->addMatcher(varDecl(hasType(isInteger())).bind("intVar"), this);
}

void MyNewCheck::check(const MatchFinder::MatchResult& Result) {
  if (const auto* VD = Result.Nodes.getNodeAs<VarDecl>("intVar")) {
    diag(VD->getLocation(), "found integer variable '%0'") << VD->getName();
  }
}
```

#### 步骤 3: 注册检查器

```cpp
// src/codelint_plugin/CodelintModule.cpp
void CodelintModule::addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
  CheckFactories.registerCheck<InitCheck>("codelint-init");
  CheckFactories.registerCheck<MyNewCheck>("codelint-mynew");  // 新增
}
```

#### 步骤 4: 添加到构建系统

```cmake
# CMakeLists.txt
add_llvm_library(codelint-plugin SHARED
    src/codelint_plugin/CodelintModule.cpp
    src/codelint_plugin/checks/InitCheck.cpp
    src/codelint_plugin/checks/MyNewCheck.cpp  # 新增
    ...
)
```

### 3.5 诊断级别

Codelint 支持三种诊断级别：

| 级别 | 触发场景 | 行为 |
|------|----------|------|
| **Warning** (默认) | 大多数初始化问题 | 报告并可自动修复 |
| **Error** | 危险布尔转换 (`int→bool`) | 报告，需手动修复 |
| **Note** | 补充信息 | 附加到主诊断 |

---

## 四、未来规划

### 4.1 "发现一例，整改一批" — 代码库级批量修复

#### 现状

当前，Codelint 主要面向单文件或手动指定的文件列表运行：

```bash
# 手动指定文件或目录
clang-tidy --load=... --checks=codelint-init --fix src/foo.cpp src/bar.cpp
```

对于大型代码库，这种方式存在以下痛点：

1. **无法全局扫描** - 难以了解整个代码库的问题分布
2. **修复效率低** - 逐个文件修复，无法批量处理
3. **进度不可追踪** - 没有可视化的修复进度

#### 未来方案

```
全代码库扫描 → 生成修复报告 → 批量 PR → 自动化合入
```

**阶段 1: 代码库扫描器**

```bash
# 扫描整个项目
codelint-scan --project=/path/to/project --output=report.json

# 输出示例 (report.json)
{
  "total_files": 1523,
  "files_with_issues": 342,
  "issues_by_type": {
    "uninitialized": 1245,
    "equals_init": 876,
    "missing_u_suffix": 234,
    "bool_conversion": 45
  },
  "heatmap": [
    {"file": "src/network/client.cpp", "issues": 67},
    {"file": "src/storage/cache.cpp", "issues": 54},
    ...
  ]
}
```

**阶段 2: 修复优先级排序**

自动根据以下因素决定修复顺序：

- **严重程度** - Error > Warning
- **影响范围** - 被调用次数多的模块优先
- **修改成本** - 改动少的优先
- **文件类型** - 实现文件优先于头文件

**阶段 3: 批量 PR 生成**

```bash
# 批量创建 PR
codelint-fix \
  --check=codelint-init \
  --scope=all \
  --create-pr \
  --group-by=severity

# 输出:
# ✅ Created PR #1234: "Auto-fix: codelint-init (Error-level)"
# ✅ Created PR #1235: "Auto-fix: codelint-init (high-frequency files)"
# ✅ Created PR #1236: "Auto-fix: codelint-init (remaining)"
```

每个 PR 聚焦一类问题或一个模块，降低 review 成本。

**阶段 4: AI 辅助审查**

对于无法自动修复的问题（如 narrowing conversion、bool conversion），提供 AI 辅助：

```bash
codelint-fix --ai-assist --review-remaining

# 对于每个未修复的问题:
# ❌ int x = 3.14;  (narrowing)
# 💡 AI 建议: int x{3}; 或改为 float x{3.14f};
# 选择: [a] 接受建议 [s] 跳过 [m] 手动编辑
```

#### 预期收益

| 指标 | 当前 | 未来 |
|------|------|------|
| 单文件修复 | ✅ 支持 | ✅ 支持 |
| 代码库扫描 | ❌ 不支持 | ✅ 热力图分布 |
| 批量修复 | ❌ 手动 | ✅ 一键修复 |
| PR 生成 | ❌ 手动 | ✅ 自动分组 |
| AI 辅助 | ❌ 无 | ✅ 智能建议 |

---

### 4.2 "部门定制化规范" — 规则适配与扩展

#### 现状

当前 Codelint 专注于通用初始化规范，所有团队使用同一套规则：

```yaml
# .clang-tidy
Checks: 'codelint-init'
```

#### 未来方案

能力矩阵升级：

| 维度 | 当前 | 未来 |
|------|------|------|
| **规则类型** | 初始化相关 | 任意 AST 模式（命名、设计、安全） |
| **配置粒度** | 全局启用/禁用 | 按目录、按文件、按团队定制 |
| **修复策略** | 内置 FixIt | 自定义修复模板 + AI 生成 |
| **报告形式** | 终端输出 | 仪表盘 + 趋势分析 |

**阶段 1: 规则 DSL (Domain-Specific Language)**

定义声明式规则语言，非 C++ 专家也能编写 checks：

```yaml
# examples/safety_department_rule.yaml
name: "安全部 - 禁止裸指针"
description: "使用智能指针代替裸指针,防止内存泄漏"
matcher: "varDecl(hasType(isPointerType()), unless(hasType(pointerType(pointee(functionType())))))"
message: "禁止裸指针，请使用 std::unique_ptr 或 std::shared_ptr"
fix:
  type: replacement
  template: "std::unique_ptr<${type}> ${name}"
severity: error
exclusions:
  - "third_party/**"
  - "legacy/**"
```

```yaml
# examples/performance_department_rule.yaml
name: "性能部 - 避免大对象按值传递"
description: "超过 64 字节的对象应当按 const 引用传递"
matcher: "functionDecl(parameterCount() > 0, hasParameter(hasType(recordDecl(hasSizeGreaterThan(64)))))"
message: "大对象应按 const 引用传递"
fix:
  type: suggestion
  template: "const ${type}& ${name}"
severity: warning
```

**阶段 2: 规则包市场**

预置常用规则包，开箱即用：

```bash
# 安装安全规范包
codelint install cert-cpp

# 安装性能规范包
codelint install performance-best-practices

# 安装 Google 风格指南
codelint install google-cpp-style

# 查看已安装包
codelint list-packages
```

**阶段 3: 规则继承与覆盖**

```
┌─────────────────────────┐
│   公司基线规则 (必须)    │
│   - 安全红线            │
│   - 内存安全            │
└───────────┬─────────────┘
            │ extends
            ▼
┌─────────────────────────┐
│  事业部规则 (额外要求)   │
│   - 编码风格            │
│   - 性能规范            │
└───────────┬─────────────┘
            │ extends
            ▼
┌─────────────────────────┐
│   项目组规则 (特殊场景)  │
│   - 嵌入式约束          │
│   - 实时性要求          │
└─────────────────────────┘
```

```yaml
# project/.codelint/config.yaml
extends:
  - company:baseline
  - department:safety
  - department:performance

rules:
  # 项目特殊覆盖
  - id: custom-realtime-constraint
    matcher: "callExpr(callee(name("sleep")))"
    message: "实时任务禁止调用 sleep()"
    severity: error
```

**阶段 4: 可视化仪表盘**

```
┌─────────────────────────────────────────────────────┐
│  Codelint Dashboard - Project XYZ                   │
├─────────────────────────────────────────────────────┤
│  📊 总体健康度     ████████░░ 85% (A)              │
│  📈 趋势          ▼ 问题数减少 23% (30 天)         │
│                                                       │
│  问题分布:                                            │
│  ├─ Error:  12  (需立即修复)                         │
│  ├─ Warning: 156 (建议修复)                          │
│  └─ Info:   45  (可选优化)                          │
│                                                       │
│  团队排名:                                           │
│  1. Team A  ████████████  98%                        │
│  2. Team B  ████████░░░░  82%                        │
│  3. Team C  ██████░░░░░░  65%                        │
└─────────────────────────────────────────────────────┘
```

---

### 4.3 "AI 检视的降维打击" — 与传统 AI Code Review 对比

#### 背景

随着 AI 编程助手普及，很多团队开始使用 Copilot、Cursor 等工具进行 Code Review。但这些工具存在本质局限：

#### 核心对比

| 维度 | 传统 AI Review (Copilot/Cursor) | Codelint |
|------|--------------------------------|----------|
| **检测原理** | LLM 模式匹配，基于训练数据概率 | Clang AST 语义分析，编译级精确 |
| **误报率** | 高 (10-30%)，大量无效建议 | 极低 (<1%)，跳过列表精准 |
| **修复可靠性** | 生成的代码可能引入新 bug | FixIt 经 AST 验证，保证语法正确 |
| **可解释性** | "AI 认为这样更好" | "违反 C++ Core Guidelines C.123" |
| **覆盖场景** | 通用建议，缺乏深度 | 专项检查 (如 initializer_list 陷阱) |
| **集成成本** | 依赖云端 AI 服务 | 本地运行，无网络需求 |
| **数据隐私** | 代码可能外泄 | 100% 本地，符合企业合规 |
| **规则定制** | 需要复杂的 prompt engineering | YAML 配置文件即可 |
| **CI/CD 集成** | 难以自动化，依赖人工判断 | 标准 exit code，原生支持流水线 |
| **成本** | 按 Token 计费，成本高 | 一次性部署，无限使用 |

#### 典型场景对比

**场景 1: 未初始化变量检测**

```cpp
int count;
process(count);
```

- **AI Review**: 可能漏报，或建议"考虑初始化"（非强制）
- **Codelint**: 100% 检出，自动修复为 `int count{}`

**场景 2: initializer_list 构造函数陷阱**

```cpp
class MyArray {
  MyArray(std::initializer_list<int>);
  MyArray(int);
};

MyArray arr = 1;
```

- **AI Review**: 大概率无法识别此陷阱
- **Codelint**: 智能跳过，避免误修复

**场景 3: 危险类型转换**

```cpp
bool success = Init();  // Init() 返回 int
```

- **AI Review**: 可能认为"语法正确，无问题"
- **Codelint**: 报告 Error，强制开发者显式处理

#### 核心优势总结

> **AI Review 是建议式的，Codelint 是强制执行的**
>
> | 工具 | 典型输出 | 行为 |
> |------|----------|------|
> | AI Review | "这里可能有问题，建议..." | 开发者自行决定是否采纳 |
> | Codelint | "这是错误，必须修复" | 不修复无法通过 CI/Pre-commit |

#### 最佳实践：AI + Codelint 组合

```
┌─────────────────────────────────────────────────────┐
│              理想开发流程                           │
├─────────────────────────────────────────────────────┤
│                                                     │
│  1. 开发者编码                                       │
│     ↓                                               │
│  2. Codelint Pre-commit (强制规范)                  │
│     - 确保不违反规范                               │
│     - 自动修复格式/初始化问题                       │
│     ↓                                               │
│  3. AI Review (优化建议)                            │
│     - 架构设计建议                                 │
│     - 性能优化点                                    │
│     - 可读性改进                                    │
│     ↓                                               │
│  4. 人工 Code Review (最终把关)                     │
│     - 业务逻辑正确性                               │
│     - 设计合理性                                    │
│     ↓                                               │
│  5. CI/CD (Codelint 作为质量门禁)                    │
│     - 新增代码必须通过所有 checks                  │
│     ↓                                               │
│  6. 合入主干                                        │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**分工明确**:
- **Codelint** - 强制执行规范，零容忍
- **AI Review** - 提供优化建议，可采纳可不采纳
- **人工 Review** - 业务逻辑和设计最终把关

---

## 五、快速上手

### 5.1 环境要求

| 组件 | 版本要求 | 安装方式 |
|------|----------|----------|
| LLVM/Clang | 21+ | Homebrew: `brew install llvm@21` |
| CMake | 3.20+ | Homebrew: `brew install cmake` |
| C++ 编译器 | C++20 支持 | 系统自带或 `brew install gcc` |

### 5.2 编译

```bash
# Clone 代码库
git clone https://github.com/your-org/codelint.git
cd codelint

# 配置 CMake (指定 LLVM 路径)
cmake -B build \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm

# 编译
cmake --build build

# 编译后产物
# build/lib/codelint-plugin.dylib  (macOS)
# build/lib/codelint-plugin.so     (Linux)
```

### 5.3 运行

#### 方式 1: 直接调用 clang-tidy

```bash
clang-tidy \
  --load=build/lib/codelint-plugin.dylib \
  --checks=codelint-init \
  --fix \
  src/**/*.cpp
```

#### 方式 2: 使用 .clang-tidy 配置文件

在项目根目录创建 `.clang-tidy`:

```yaml
# .clang-tidy
---
Checks: 'codelint-init'
HeaderFilterRegex: '.*'
WarningsAsErrors: 'codelint-init'
CheckOptions:
  - key: InitCheck.StrictBool
    value: true
```

然后运行:

```bash
clang-tidy \
  --load=build/lib/codelint-plugin.dylib \
  -p build \
  src/**/*.cpp
```

#### 方式 3: 集成到构建系统

**CMake 集成:**

```cmake
# CMakeLists.txt
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
    ${CLANG_TIDY}
    --load=${PROJECT_SOURCE_DIR}/build/lib/codelint-plugin.dylib
    --checks=codelint-init
  )
endif()
```

**Makefile 集成:**

```makefile
.PHONY: lint
lint:
	clang-tidy --load=./build/lib/codelint-plugin.dylib \
	           --checks=codelint-init \
	           --fix \
	           $(shell find src -name '*.cpp')
```

### 5.4 配置选项

Codelint 支持通过 `.clang-tidy` 配置行为:

```yaml
# .clang-tidy
Checks: 'codelint-init'
CheckOptions:
  # 严格布尔转换检查 (默认: true)
  - key: InitCheck.StrictBool
    value: true

  # 启用 U 后缀检查 (默认: true)
  - key: InitCheck.CheckUnsignedSuffix
    value: true

  # 头文件也检查 (默认: true)
  - key: InitCheck.CheckHeaders
    value: true
```

### 5.5 典型工作流

```bash
# 1. 首次扫描 - 了解问题分布
clang-tidy --load=... --checks=codelint-init src/*.cpp > report.txt

# 2. 自动修复 - 处理所有可修复问题
clang-tidy --load=... --checks=codelint-init --fix src/*.cpp

# 3. 人工审查 - 处理 Error 级别问题
# (bool conversion, narrowing 等需手动处理)

# 4. 提交前检查 - 确保无新增问题
git diff --name-only | xargs \
  clang-tidy --load=... --checks=codelint-init --error-on-warnings
```

### 5.6 故障排查

**问题 1: 插件加载失败**

```
error: unable to load plugin 'build/lib/codelint-plugin.dylib'
```

解决方案:
- 确认 LLVM 版本匹配 (必须 21)
- 确认插件已编译完成
- 检查路径是否正确

**问题 2: 误报问题**

如果 Codelint 报告了你认为是误报的问题:

1. 检查是否在跳过列表中
2. 如果是合理场景，提交 Issue 请求加入跳过列表
3. 临时方案：使用 `// NOLINT(codelint-init)` 抑制

```cpp
int x;  // NOLINT(codelint-init) - 特殊原因
```

---

## 附录

### A. 常见问题 (FAQ)

**Q: Codelint 会修改我的代码行为吗？**

A: 不会。所有自动修复都保持语义不变:
- `int x;` → `int x{}` (零初始化，更安全)
- `int x = 5;` → `int x{5};` (等价值初始化)
- `unsigned u = 100;` → `unsigned u{100U};` (等值，添加后缀)

**Q: 如何禁用某些检查？**

A: 使用 `// NOLINT` 注释:

```cpp
int raw_ptr;  // NOLINT(codelint-init) - 特殊场景
```

或在 `.clang-tidy` 中配置:

```yaml
Checks: '-codelint-init'  # 禁用
```

**Q: 支持 C 语言吗？**

A: 当前仅支持 C++。C 语言版本可能在后续版本中考虑。

**Q: 如何贡献代码？**

A: 欢迎提交 Pull Request! 特别是:
- 新的检查器
- 跳过列表补充
- 性能优化
- 文档改进

### B. 参考资料

- [LLVM Documentation](https://llvm.org/docs/)
- [Clang-tidy Plugin Guide](https://clang.llvm.org/extra/clang-tidy/)
- [AST Matcher Reference](https://clang.llvm.org/docs/LibASTMatchersReference.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

### C. 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

**版本**: v3.0
**最后更新**: 2026-04-16
**维护者**: Codelint Team
**联系方式**: [GitHub Issues](https://github.com/your-org/codelint/issues)
