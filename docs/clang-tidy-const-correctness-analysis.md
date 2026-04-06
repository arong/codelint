# clang-tidy `misc-const-correctness` 移植分析

## 📋 概述

本文档分析了 clang-tidy 的 `misc-const-correctness` 检查实现，用于指导将其移植到 codelint 项目。

---

## 🏗️ 架构对比

### clang-tidy 架构

```cpp
class ConstCorrectnessCheck : public ClangTidyCheck {
  // 基于 AST Matchers 的声明式模式
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  // 关键工具类
  llvm::DenseMap<const Stmt *, std::unique_ptr<ExprMutationAnalyzer>> ScopesCache;
  ExprMutationAnalyzer::Memoized ParamMutationAnalyzerMemoized;
};
```

### codelint 架构

```cpp
class ConstChecker : public LintChecker,
                     public clang::RecursiveASTVisitor<ConstChecker> {
  // 基于 RecursiveASTVisitor 的命令式模式
  bool VisitVarDecl(clang::VarDecl* VD);
  bool VisitBinaryOperator(clang::BinaryOperator* BO);
  void runOnAST(clang::ASTContext* Context);

  // 简单的数据结构
  std::unordered_map<std::string, VarInfo> variables_;
  std::unordered_set<std::string> modified_vars_;
};
```

**关键差异**：
| 特性 | clang-tidy | codelint |
|------|-----------|----------|
| **AST 遍历模式** | AST Matchers（声明式） | RecursiveASTVisitor（命令式） |
| **变更分析** | `ExprMutationAnalyzer` | 手动检测 |
| **配置系统** | `Options.get()` | ❌ 无 |
| **参数分析** | ✅ 支持 | ❌ 不支持 |
| **指针分析** | ✅ 两级（指针+指向值） | ⚠️ 有限支持 |

---

## 🔑 核心实现要点

### 1. 变量变更检测（最关键）

**clang-tidy 的方法**：使用 `ExprMutationAnalyzer`

```cpp
// 来自 clang/Analysis/Analyses/ExprMutationAnalyzer.h
bool isMutated(const VarDecl *Variable, const Stmt *Scope,
               const FunctionDecl *Func, ASTContext *Context) {
  // 对参数使用专门的分析器
  if (const auto *Param = dyn_cast<ParmVarDecl>(Variable)) {
    return FunctionParmMutationAnalyzer::getFunctionParmMutationAnalyzer(
               *Func, *Context, ParamMutationAnalyzerMemoized)
        ->isMutated(Param);
  }

  // 对局部变量使用通用分析器
  registerScope(Scope, Context);
  return ScopesCache[Scope]->isMutated(Variable);
}
```

**codelint 当前的实现**：手动检测

```cpp
// 当前实现（简化版）
bool VisitBinaryOperator(clang::BinaryOperator* BO) {
  if (BO->isAssignmentOp()) {
    // 检查左侧是否是已追踪的变量
    if (auto *DRE = dyn_cast<DeclRefExpr>(BO->getLHS())) {
      if (auto *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        modified_vars_.insert(getVarKey(VD));
      }
    }
  }
}
```

**移植建议**：
- ⚠️ **高优先级**：移植 `ExprMutationAnalyzer` 到 codelint
- 这是 clang 的一部分，位于 `clang/Analysis/Analyses/ExprMutationAnalyzer.h`
- 提供 `isMutated()` API，检测变量是否在给定作用域内被修改

---

### 2. AST Matchers 模式

**clang-tidy 的声明式匹配**：

```cpp
void ConstCorrectnessCheck::registerMatchers(MatchFinder *Finder) {
  // 匹配局部变量
  const auto LocalValDecl =
      varDecl(isLocal(), hasInitializer(),
              unless(ConstType), unless(TemplateType), unless(AllowedType));

  // 匹配函数作用域
  const auto FunctionScope =
      functionDecl(hasBody(stmt(forEachDescendant(
          declStmt(containsAnyDeclaration(LocalValDecl.bind("value")))
              .bind("decl-stmt")))));

  Finder->addMatcher(FunctionScope, this);
}
```

**codelint 的命令式遍历**：

```cpp
bool VisitVarDecl(clang::VarDecl* VD) {
  // 手动过滤条件
  if (isInSystemHeader(VD)) return true;
  if (shouldSkipStaticVariable(VD)) return true;

  // 手动分析
  if (!VD->getInit()) {
    checkUninitialized(VD);
  }
}
```

**移植建议**：
- ✅ **可以保持当前架构**：RecursiveASTVisitor 模式更简单直接
- ⚠️ **考虑混合模式**：在 visitor 内部使用 matchers 过滤
- 💡 **优势**：AST Matchers 提供更强大的模式匹配能力

---

### 3. 函数参数分析

**clang-tidy 的实现**：

```cpp
void ConstCorrectnessCheck::registerMatchers(MatchFinder *Finder) {
  if (AnalyzeParameters) {
    // 匹配函数参数（引用和指针类型）
    const auto ParamMatcher =
        parmVarDecl(unless(CommonExcludeTypes),
                    anyOf(hasType(referenceType()),
                          hasType(pointerType())))
            .bind("value");

    Finder->addMatcher(FunctionWithParams, this);
  }
}
```

**codelint 当前**：❌ 不支持

**移植建议**：
- ✅ **高优先级**：添加参数分析
- 实现 `VisitParmVarDecl()` 或在 `VisitVarDecl()` 中检查 `ParmVarDecl`
- 关键：需要 `FunctionParmMutationAnalyzer` 来检测参数是否被修改

---

### 4. 指针的两级分析

**clang-tidy 的实现**：

```cpp
// 两个独立的检查
if (VC == VariableCategory::Pointer && AnalyzePointers) {
  // 检查1：指针本身是否可以 const
  if (WarnPointersAsValues && !VT.isConstQualified()) {
    CheckValue();  // 建议 int* const ptr
  }

  // 检查2：指向的值是否可以 const
  if (WarnPointersAsPointers) {
    CheckPointee();  // 建议 const int* ptr
  }
}
```

**codelint 当前**：⚠️ 仅检测指针指向的值

**移植建议**：
- ✅ **中优先级**：添加配置选项
- ```yaml
  WarnPointersAsValues: false  # 建议 const 指针本身
  WarnPointersAsPointers: true # 建议 const 指向值
  ```

---

### 5. 配置系统

**clang-tidy 的配置**：

```cpp
class ConstCorrectnessCheck : public ClangTidyCheck {
  const bool AnalyzePointers;
  const bool AnalyzeReferences;
  const bool AnalyzeValues;
  const bool AnalyzeParameters;
  const std::vector<StringRef> AllowedTypes;

  // 从配置文件读取
  ConstCorrectnessCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      AnalyzePointers(Options.get("AnalyzePointers", true)),
      AnalyzeReferences(Options.get("AnalyzeReferences", true)),
      // ...
```

**codelint 当前**：❌ 硬编码

**移植建议**：
- ✅ **高优先级**：添加配置文件支持
- 使用 YAML 格式：`.codelint.yml`
- ```yaml
    const_checker:
      analyze_pointers: true
      analyze_references: true
      analyze_parameters: true
      allowed_types: ["smart_ptr.*", "RefPtr"]
    ```

---

## 📦 需要移植的关键组件

### 必需组件（高优先级）

1. **`ExprMutationAnalyzer`** ⭐⭐⭐
   - 路径：`clang/Analysis/Analyses/ExprMutationAnalyzer.h`
   - 功能：精确检测变量是否被修改
   - 难度：中等（需要移植依赖）

2. **`FunctionParmMutationAnalyzer`** ⭐⭐⭐
   - 功能：检测函数参数是否被修改
   - 难度：中等（依赖 ExprMutationAnalyzer）

3. **配置系统** ⭐⭐
   - 功能：支持 YAML 配置文件
   - 难度：简单（可以使用 yaml-cpp）

### 可选组件（中优先级）

4. **AST Matchers 辅助** ⭐
   - 功能：声明式模式匹配
   - 难度：简单（Clang 已提供）

5. **类型过滤系统** ⭐
   - 功能：`AllowedTypes` 正则表达式过滤
   - 难度：简单

---

## 🛠️ 移植步骤建议

### Phase 1: 核心功能增强（2-3天）

```cpp
// 1. 移植 ExprMutationAnalyzer
#include "analysis/expr_mutation_analyzer.h"  // 新文件

class ConstChecker {
  std::unique_ptr<ExprMutationAnalyzer> mutation_analyzer_;

  bool isVariableModified(const clang::VarDecl* VD,
                          const clang::Stmt* Scope) {
    return mutation_analyzer_->isMutated(VD);
  }
};
```

### Phase 2: 参数分析（1-2天）

```cpp
// 2. 添加参数分析
bool VisitParmVarDecl(clang::ParmVarDecl* PVD) {
  if (!AnalyzeParameters_) return true;

  // 只分析引用和指针参数
  auto type = PVD->getType();
  if (!type->isReferenceType() && !type->isPointerType())
    return true;

  // 使用 FunctionParmMutationAnalyzer 检测修改
  // ...
}
```

### Phase 3: 配置系统（1天）

```cpp
// 3. 添加配置支持
class ConstChecker {
  struct Config {
    bool analyze_pointers = true;
    bool analyze_references = true;
    bool analyze_parameters = false;  // 默认关闭
    std::vector<std::string> allowed_types;
  };

  Config config_;

  void loadConfig(const std::string& filepath);
};
```

---

## 📊 功能对比矩阵

| 功能 | clang-tidy | codelint (当前) | codelint (目标) |
|------|-----------|----------------|----------------|
| **局部变量检测** | ✅ ExprMutationAnalyzer | ⚠️ 手动检测 | ✅ ExprMutationAnalyzer |
| **参数检测** | ✅ FunctionParmMutationAnalyzer | ❌ | ✅ FunctionParmMutationAnalyzer |
| **指针分析** | ✅ 两级分析 | ⚠️ 单级 | ✅ 两级分析 |
| **配置系统** | ✅ YAML | ❌ | ✅ YAML |
| **类型过滤** | ✅ 正则表达式 | ❌ | ✅ 正则表达式 |
| **模板处理** | ✅ 缓存机制 | ❌ | ✅ 缓存机制 |
| **自动修复** | ✅ FixIt hints | ✅ --fix | ✅ --fix |

---

## 💡 关键代码片段

### ExprMutationAnalyzer 使用示例

```cpp
// 来自 clang-tidy 的用法
void ConstCorrectnessCheck::check(const MatchResult &Result) {
  const auto *Variable = Result.Nodes.getNodeAs<VarDecl>("value");
  const auto *Scope = Result.Nodes.getNodeAs<Stmt>("scope");

  // 核心检测逻辑
  if (!isMutated(Variable, Scope, Function, Context)) {
    // 变量未被修改，建议 const
    diag(Variable->getBeginLoc(),
         "variable %0 can be declared 'const'")
        << Variable;
  }
}
```

### 指针分析示例

```cpp
void ConstCorrectnessCheck::check(const MatchResult &Result) {
  const QualType VT = Variable->getType();

  if (VT->isPointerType()) {
    // 检查1：指针本身
    if (WarnPointersAsValues && !VT.isConstQualified()) {
      // 建议：int* const ptr = &value;
      addConstFixits(Diag, Variable, Context,
                     Qualifiers::Const,
                     QualifierTarget::Value);
    }

    // 检查2：指向的值
    if (WarnPointersAsPointers) {
      if (!VT->getPointeeType().isConstQualified()) {
        // 建议：const int* ptr = &value;
        addConstFixits(Diag, Variable, Context,
                       Qualifiers::Const,
                       QualifierTarget::Pointee);
      }
    }
  }
}
```

---

## 🎯 下一步行动

### 立即行动
1. ✅ **复制源代码**：将 clang-tidy 的 `ConstCorrectnessCheck.cpp/h` 保存为参考
2. ✅ **研究 ExprMutationAnalyzer**：理解其实现原理
3. ✅ **创建原型**：在新分支实现基础功能

### 本周目标
1. 移植 `ExprMutationAnalyzer` 核心逻辑
2. 添加函数参数分析
3. 实现配置系统原型

### 本月目标
1. 完整的两级指针分析
2. 类型过滤系统
3. 模板实例化处理

---

## 📚 参考资源

- **clang-tidy 源代码**：
  - `clang-tools-extra/clang-tidy/misc/ConstCorrectnessCheck.cpp`
  - `clang-tools-extra/clang-tidy/misc/ConstCorrectnessCheck.h`

- **Clang 分析工具**：
  - `clang/Analysis/Analyses/ExprMutationAnalyzer.h`
  - `clang/Analysis/Analyses/ExprMutationAnalyzer.cpp`

- **AST Matchers**：
  - `clang/ASTMatchers/ASTMatchers.h`
  - `clang/ASTMatchers/ASTMatchFinder.h`

- **配置系统**：
  - `llvm/Support/YAMLTraits.h`

---

## ⚠️ 注意事项

1. **许可证兼容性**：
   - LLVM 使用 Apache 2.0 + LLVM Exception
   - 需要保留版权声明和许可证文本

2. **依赖关系**：
   - `ExprMutationAnalyzer` 依赖其他 Clang 分析库
   - 可能需要移植部分 Analysis 模块

3. **性能考虑**：
   - `ExprMutationAnalyzer` 会缓存分析结果
   - 对大型函数可能有内存开销

4. **模板处理**：
   - clang-tidy 有专门的模板实例化缓存
   - 避免对同一变量重复警告

---

*文档创建日期：2026-04-06*
*基于 clang-tidy LLVM main 分支（commit: 2026-01）*
