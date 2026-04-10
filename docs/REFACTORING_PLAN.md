# Codelint Refactoring Plan: LibTooling Binary → Clang-Tidy Plugin

## Executive Summary

This document details the transformation of codelint from a standalone LibTooling binary to a clang-tidy plugin architecture, following user-mandated decisions to simplify the architecture and adopt AST Matchers.

---

## 1. Architecture Analysis

### 1.1 Current Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     CURRENT ARCHITECTURE                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────┐    ┌──────────────┐    ┌─────────────────┐    │
│  │ main.cpp │───▶│ CLI11 Parser │───▶│ Command Router  │    │
│  └──────────┘    └──────────────┘    └─────────────────┘    │
│                                              │               │
│              ┌───────────────────────────────┼───────────┐   │
│              │                               │           │   │
│              ▼                               ▼           ▼   │
│  ┌─────────────────┐  ┌───────────────┐  ┌─────────────┐   │
│  │ cmd_check_init  │  │cmd_find_global│  │cmd_find_    │   │
│  │                 │  │               │  │singleton    │   │
│  └─────────────────┘  └───────────────┘  └─────────────┘   │
│              │                               │           │   │
│              ▼                               ▼           ▼   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              CheckerFactory                         │   │
│  └─────────────────────────────────────────────────────┘   │
│              │                               │           │   │
│              ▼                               ▼           ▼   │
│  ┌─────────────────┐  ┌───────────────┐  ┌─────────────┐   │
│  │  InitChecker    │  │ GlobalChecker │  │SingletonChk │   │
│  │  (LibTooling)   │  │ (LibTooling)  │  │(LibTooling) │   │
│  └─────────────────┘  └───────────────┘  └─────────────┘   │
│              │                               │           │   │
│              └───────────────────────────────┼───────────┘   │
│                                              ▼               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │           LibTooling Infrastructure                 │   │
│  │  ┌─────────────────┐  ┌────────────────────────┐   │   │
│  │  │FrontendAction   │  │ClangTool + FixedCompDB │   │   │
│  │  │Factory          │  │                        │   │   │
│  │  └─────────────────┘  └────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                              │               │
│              ┌───────────────────────────────┼───────────┐   │
│              │                               │           │   │
│              ▼                               ▼           ▼   │
│  ┌─────────────────┐  ┌───────────────┐  ┌─────────────┐   │
│  │ IssueReporter   │  │   FixApplier  │  │   GitScope  │   │
│  │ (JSON/SARIF/    │  │ (String-based │  │ (libgit2)   │   │
│  │  Text)          │  │  replacement) │  │             │   │
│  └─────────────────┘  └───────────────┘  └─────────────┘   │
│                                                               │
│  DEPENDENCIES: CLI11, libgit2, RapidJSON, LLVM/Clang        │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Target Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     TARGET ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  clang-tidy binary                                           │
│      │                                                        │
│      ▼                                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Plugin Loading (--load)                   │   │
│  └──────────────────────────────────────────────────────┘   │
│      │                                                        │
│      ▼                                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              CodelintModule                            │   │
│  │  ┌────────────────────────────────────────────────┐  │   │
│  │  │  ClangTidyModule::addCheckFactories()          │  │   │
│  │  │    - Register InitCheck                        │  │   │
│  │  │    - Register GlobalCheck                      │  │   │
│  │  │    - Register SingletonCheck                   │  │   │
│  │  └────────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────┘   │
│      │                                                        │
│      ├──────────────────────┬─────────────────────          │
│      ▼                      ▼                      ▼        │
│  ┌──────────────┐  ┌────────────────┐  ┌──────────────┐    │
│  │  InitCheck   │  │  GlobalCheck   │  │SingletonCheck│    │
│  │              │  │                │  │              │    │
│  │ ┌──────────┐ │  │ ┌────────────┐ │  │ ┌──────────┐ │    │
│  │ │register- │ │  │ │register-   │ │  │ │register- │ │    │
│  │ │Matchers()│ │  │ │Matchers()  │ │  │ │Matchers()│ │    │
│  │ └──────────┘ │  │ └────────────┘ │  │ └──────────┘ │    │
│  │      │       │  │      │         │  │      │       │    │
│  │      ▼       │  │      ▼         │  │      ▼       │    │
│  │ ┌──────────┐ │  │ ┌────────────┐ │  │ ┌──────────┐ │    │
│  │ │  check() │ │  │ │  check()   │ │  │ │  check() │ │    │
│  │ │          │ │  │ │            │ │  │ │          │ │    │
│  │ │diag() +  │ │  │ │diag()      │ │  │ │diag()    │ │    │
│  │ │FixItHint │ │  │ │            │ │  │ │          │ │    │
│  │ └──────────┘ │  │ └────────────┘ │  │ └──────────┘ │    │
│  └──────────────┘  └────────────────┘  └──────────────┘    │
│                                                               │
│  DEPENDENCIES: LLVM/Clang (clang-tidy module only)          │
│                                                               │
│  OUTPUT: clang-tidy built-in formats (YAML, JSON, etc.)     │
│  FIXES: clang-tidy built-in --fix mechanism                 │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### 1.3 Deleted Components

| Category | Component | Reason |
|----------|-----------|--------|
| CLI | `main.cpp` | Replaced by clang-tidy invocation |
| CLI | `CLI11` dependency | clang-tidy handles CLI |
| CLI | `cmd_check_init.cpp` | Command routing obsolete |
| CLI | `cmd_find_global.cpp` | Command routing obsolete |
| CLI | `cmd_find_singleton.cpp` | Command routing obsolete |
| CLI | `cmd_utils.cpp/h` | Helper functions obsolete |
| Output | `IssueReporter` | clang-tidy built-in output formats |
| Output | `--output-sarif` | clang-tidy supports SARIF |
| Output | `--output-json` | clang-tidy JSON format |
| Output | Custom text format | clang-tidy standard format |
| Git | `GitScope` class | Feature deleted per user decision |
| Git | `libgit2` dependency | Feature deleted |
| Git | `--scope` option | Feature deleted |
| Fix | `FixApplier` class | clang-tidy --fix mechanism |
| Fix | `--fix` / `--inplace` | clang-tidy --fix option |
| Fix | `suppress_constant` | Deleted per user decision |
| Infrastructure | `LintChecker` base | Replaced by ClangTidyCheck |
| Infrastructure | `CheckerFactory` | Replaced by ClangTidyModule |
| Infrastructure | `LintAction/Consumer` | clang-tidy handles AST |
| Infrastructure | `LintVisitor` | AST Matchers replace visitor |
| Infrastructure | `LintResult/Issue` | clang-tidy diagnostics |
| Infrastructure | `LintConfig` | clang-tidy configuration |
| Infrastructure | `LintRunner` | clang-tidy runner |

---

## 2. Detailed Component Analysis

### 2.1 InitChecker Complexity (Highest Risk)

**Current Implementation (init_checker.cpp:719 lines)**:
- Uses `RecursiveASTVisitor` pattern
- CFG-based modification tracking (lines ~400-600)
- Multiple skip predicates (10 helper methods)
- String-based fix generation
- Const/constexpr suggestion logic

**Challenges for AST Matcher Migration**:

```
┌─────────────────────────────────────────────────────────┐
│  CRITICAL: CFG-dependent checks cannot use Matchers     │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Current CFG-based checks:                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │ • const/constexpr suggestions                     │ │
│  │   - Requires tracking variable modifications      │ │
│  │   - Needs flow-sensitive analysis                 │ │
│  │   - Manual traversal + CFG traversal              │ │
│  │                                                   │ │
│  │ • "Variable is never modified after init"         │ │
│  │   - Requires control flow analysis                │ │
│  │   - Cannot express in pattern matcher             │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
│  Matcher-applicable checks:                             │
│  ┌───────────────────────────────────────────────────┐ │
│  │ ✓ Uninitialized variable (pattern-based)          │ │
│  │ ✓ = initialization (pattern-based)                │ │
│  │ ✓ Missing U suffix (pattern-based)                │ │
│  │ ✓ Field uninitialized (pattern-based)             │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**Resolution Strategy**:
1. **Phase 1**: Implement pattern-based checks with Matchers
2. **Phase 2**: Evaluate clang's dataflow analysis framework
3. **Phase 3**: If dataflow unavailable, implement manual flow analysis in `check()`

### 2.2 GlobalChecker (Simple Migration)

**Current Implementation (global_checker.cpp:229 lines)**:
- Pure pattern detection
- No CFG dependency
- Straightforward `VisitVarDecl` pattern
- Already fits matcher paradigm

**Matcher Complexity**: LOW (1-2 hours)

### 2.3 SingletonChecker (Simple Migration)

**Current Implementation (singleton_checker.cpp:248 lines)**:
- Pattern-based detection
- Meyer's Singleton pattern (static local + return ref)
- No CFG dependency
- Straightforward pattern match

**Matcher Complexity**: LOW (1-2 hours)

---

## 3. Phase-by-Phase Implementation Plan

### Phase 1: Infrastructure Setup

**Duration**: 4-6 hours
**Dependencies**: None (foundation for all phases)

#### 1.1 Create Target Directory Structure

```bash
mkdir -p src/codelint_plugin
mkdir -p src/codelint_plugin/checks
mkdir -p src/codelint_plugin/utils
mkdir -p include/codelint/checks
mkdir -p include/codelint/utils
mkdir -p docs/check-docs
mkdir -p tests/check-tests
mkdir -p tests/integration
```

**File Operations**:

| Action | Path | Purpose |
|--------|------|---------|
| CREATE | `src/codelint_plugin/CodelintModule.cpp` | Plugin registration |
| CREATE | `include/codelint/CodelintModule.h` | Module header |
| CREATE | `CMakeLists.txt.clangtidy` | New build config |
| CREATE | `.clang-tidy` | Example configuration |

#### 1.2 Plugin Registration Architecture

```cpp
// src/codelint_plugin/CodelintModule.cpp

#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "codelint/checks/InitCheck.h"
#include "codelint/checks/GlobalCheck.h"
#include "codelint/checks/SingletonCheck.h"

namespace clang::tidy {
namespace codelint {

class CodelintModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
    CheckFactories.registerCheck<InitCheck>("codelint-init");
    CheckFactories.registerCheck<GlobalCheck>("codelint-global");
    CheckFactories.registerCheck<SingletonCheck>("codelint-singleton");
  }
};

} // namespace codelint
} // namespace clang::tidy

// Register the module
static clang::tidy::ClangTidyModuleRegistry::Add<clang::tidy::codelint::CodelintModule>
    X("codelint-module", "Adds codelint checks: init, global, singleton");
```

#### 1.3 CMake Plugin Build Configuration

```cmake
# CMakeLists.txt.clangtidy

cmake_minimum_required(VERSION 3.20)
project(codelint-plugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM/Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# clang-tidy specific libraries
llvm_map_components_to_libnames(llvm_libs
    support
    clang-tidy
    clang-ast
    clang-ast-matchers
    clang-basic
    clang-lex
    clang-frontend
    clang-tooling
)

# Plugin shared library
add_library(codelint-plugin SHARED
    src/codelint_plugin/CodelintModule.cpp
    src/codelint_plugin/checks/InitCheck.cpp
    src/codelint_plugin/checks/GlobalCheck.cpp
    src/codelint_plugin/checks/SingletonCheck.cpp
)

target_include_directories(codelint-plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(codelint-plugin PRIVATE
    ${llvm_libs}
)

# Install plugin
install(TARGETS codelint-plugin LIBRARY DESTINATION lib/clang-tidy)
```

#### 1.4 Verification Criteria

| Test | Command | Expected |
|------|---------|----------|
| Plugin builds | `cmake --build .` | SUCCESS |
| Plugin loads | `clang-tidy --load=./codelint-plugin.so --list-checks` | Shows codelint-* |
| Plugin registered | `clang-tidy --checks='codelint-*' --list-checks` | 3 checks listed |

**Tasks**:

| Task ID | Description | Category | Skill | Dependencies | Duration |
|---------|-------------|----------|-------|--------------|----------|
| P1-T1 | Create directory structure | setup | none | none | 15m |
| P1-T2 | Write CodelintModule.cpp | implementation | clang-tidy | P1-T1 | 30m |
| P1-T3 | Create CMake config | build | cmake | P1-T1 | 30m |
| P1-T4 | Configure include paths | build | cmake | P1-T3 | 15m |
| P1-T5 | Test plugin loading | verification | testing | P1-T4 | 30m |
| P1-T6 | Create .clang-tidy example | documentation | docs | P1-T5 | 15m |

---

### Phase 2: Checker Rewriting (Parallel Execution)

**Duration**: 12-16 hours (can parallelize across 3 checkers)
**Dependencies**: Phase 1 complete

#### 2.1 GlobalCheck Migration (Lowest Complexity)

**Source Analysis**:

```cpp
// Current: global_checker.cpp:99-109
bool GlobalChecker::VisitVarDecl(clang::VarDecl* VD) {
  if (!VD || !Context_) return true;

  if (isGlobalVariable(VD) && !isInSystemHeader(VD) && !isExternDeclaration(VD)) {
    reportGlobalVariable(VD);
  }
  return true;
}

// Predicate chain (lines 111-149):
// - !ParmVarDecl
// - !isLocalVarDecl && !isStaticLocal
// - !isa<RecordDecl>(DC)
// - hasGlobalStorage()
// - isFileVarDecl()
```

**Matcher Implementation**:

```cpp
// include/codelint/checks/GlobalCheck.h

#pragma once
#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy {
namespace codelint {

class GlobalCheck : public ClangTidyCheck {
public:
  GlobalCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }
};

} // namespace codelint
} // namespace clang::tidy
```

```cpp
// src/codelint_plugin/checks/GlobalCheck.cpp

#include "codelint/checks/GlobalCheck.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

namespace clang::tidy {
namespace codelint {

void GlobalCheck::registerMatchers(MatchFinder* Finder) {
  // Match global variables at file scope
  Finder->addMatcher(
    varDecl(
      hasGlobalStorage(),
      unless(isStaticLocal()),
      unless(hasAncestor(functionDecl())),
      unless(hasAncestor(recordDecl())),
      unless(isExternDeclaration()),
      unless(parmVarDecl()),
      unless(isExpansionInSystemHeader())
    ).bind("global"),
    this
  );
}

void GlobalCheck::check(const MatchFinder::MatchResult& Result) {
  const auto* VD = Result.Nodes.getNodeAs<VarDecl>("global");
  if (!VD) return;

  diag(VD->getLocation(), "global variable '%0' detected")
    << VD->getName()
    << "Consider using a singleton or dependency injection pattern";
}

} // namespace codelint
} // namespace clang::tidy
```

#### 2.2 SingletonCheck Migration

**Source Analysis**:

```cpp
// Current: singleton_checker.cpp:131-185
// Meyer's Singleton pattern detection:
// 1. Function returns reference
// 2. Contains static local variable
// 3. Returns reference to that static local
```

**Matcher Implementation**:

```cpp
// include/codelint/checks/SingletonCheck.h

#pragma once
#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy {
namespace codelint {

class SingletonCheck : public ClangTidyCheck {
public:
  SingletonCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace codelint
} // namespace clang::tidy
```

```cpp
// src/codelint_plugin/checks/SingletonCheck.cpp

#include "codelint/checks/SingletonCheck.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

namespace clang::tidy {
namespace codelint {

void SingletonCheck::registerMatchers(MatchFinder* Finder) {
  // Meyer's Singleton: function returning reference to static local
  Finder->addMatcher(
    functionDecl(
      returns(referenceType()),
      hasBody(compoundStmt(
        hasDescendant(
          varDecl(isStaticStorageClass()).bind("static_local")
        ),
        hasDescendant(
          returnStmt(has(
            declRefExpr(to(varDecl(equalsBoundNode("static_local"))))
          ))
        )
      )),
      unless(isExpansionInSystemHeader())
    ).bind("singleton"),
    this
  );
}

void SingletonCheck::check(const MatchFinder::MatchResult& Result) {
  const auto* FD = Result.Nodes.getNodeAs<FunctionDecl>("singleton");
  const auto* VD = Result.Nodes.getNodeAs<VarDecl>("static_local");

  if (!FD || !VD) return;

  diag(FD->getLocation(), "Meyer's Singleton pattern detected in '%0'")
    << FD->getName()
    << "Returns reference to static local '" << VD->getName() << "'";
}

} // namespace codelint
} // namespace clang::tidy
```

#### 2.3 InitCheck Migration (Highest Complexity)

**Multi-part Matcher Strategy**:

```cpp
// include/codelint/checks/InitCheck.h

#pragma once
#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy {
namespace codelint {

class InitCheck : public ClangTidyCheck {
public:
  InitCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

private:
  void checkUninitialized(const VarDecl* VD, ASTContext* Ctx);
  void checkEqualsInit(const VarDecl* VD, ASTContext* Ctx);
  void checkUnsignedSuffix(const VarDecl* VD, ASTContext* Ctx);

  // Skip predicates (mirrored from current implementation)
  bool shouldSkipAuto(const VarDecl* VD);
  bool shouldSkipForLoop(const VarDecl* VD);
  bool shouldSkipUnion(const VarDecl* VD);
  bool shouldSkipEnumClass(const VarDecl* VD);
  bool shouldSkipExtern(const VarDecl* VD);
  bool shouldSkipException(const VarDecl* VD);
};

} // namespace codelint
} // namespace clang::tidy
```

```cpp
// src/codelint_plugin/checks/InitCheck.cpp

#include "codelint/checks/InitCheck.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy {
namespace codelint {

void InitCheck::registerMatchers(MatchFinder* Finder) {
  // Matcher 1: Uninitialized variables
  Finder->addMatcher(
    varDecl(
      unless(hasInitializer(anything())),
      unless(isStaticStorageClass()),
      unless(parmVarDecl()),
      unless(hasAncestor(functionDecl(isImplicit()))),
      unless(isExpansionInSystemHeader()),
      unless(hasType(autoType())),
      unless(hasAncestor(cxxForRangeStmt())),
      unless(hasAncestor(forStmt(hasLoopInit(varDecl())))),
      // Union skip
      unless(hasAncestor(recordDecl(isUnion()))),
      // Exception skip
      unless(hasAncestor(cxxCatchStmt()))
    ).bind("uninit"),
    this
  );

  // Matcher 2: Equals initialization (should use braces)
  Finder->addMatcher(
    varDecl(
      hasInitializer(expr()),
      unless(hasInitializer(initListExpr())),
      unless(hasType(autoType())),
      unless(hasAncestor(cxxForRangeStmt())),
      unless(isExpansionInSystemHeader()),
      unless(hasAncestor(recordDecl(isUnion()))),
      hasInitStyle(VarDecl::CInit)
    ).bind("equals_init"),
    this
  );

  // Matcher 3: Unsigned without suffix
  Finder->addMatcher(
    varDecl(
      hasType(hasCanonicalType(isUnsignedInteger())),
      hasInitializer(integerLiteral()),
      unless(isExpansionInSystemHeader())
    ).bind("unsigned"),
    this
  );
}

void InitCheck::check(const MatchFinder::MatchResult& Result) {
  ASTContext* Ctx = Result.Context;

  // Handle uninitialized
  if (const auto* VD = Result.Nodes.getNodeAs<VarDecl>("uninit")) {
    if (!shouldSkipAuto(VD) && !shouldSkipForLoop(VD) &&
        !shouldSkipUnion(VD) && !shouldSkipExtern(VD)) {
      checkUninitialized(VD, Ctx);
    }
  }

  // Handle equals init
  if (const auto* VD = Result.Nodes.getNodeAs<VarDecl>("equals_init")) {
    checkEqualsInit(VD, Ctx);
  }

  // Handle unsigned
  if (const auto* VD = Result.Nodes.getNodeAs<VarDecl>("unsigned")) {
    checkUnsignedSuffix(VD, Ctx);
  }
}

void InitCheck::checkUninitialized(const VarDecl* VD, ASTContext* Ctx) {
  auto Diag = diag(VD->getLocation(), "uninitialized variable '%0'");
  Diag << VD->getName();

  // Fix: Add {} initializer
  SourceLocation EndLoc = VD->getEndLoc();
  Diag << FixItHint::CreateInsertion(EndLoc, "{}");
}

void InitCheck::checkEqualsInit(const VarDecl* VD, ASTContext* Ctx) {
  SourceManager& SM = Ctx->getSourceManager();
  const LangOptions& LO = Ctx->getLangOpts();

  // Get the initializer expression
  Expr* Init = VD->getInit();
  SourceRange InitRange = Init->getSourceRange();

  // Get the "=" token range
  SourceLocation EqualLoc = Lexer::getLocForEndOfToken(
    VD->getLocation(), 0, SM, LO
  );

  auto Diag = diag(VD->getLocation(),
    "variable '%0' uses '=' initialization; prefer '{}' syntax");
  Diag << VD->getName();

  // Fix: Replace = with { and add }
  std::string Replacement = "{" +
    Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SM, LO).str() + "}";

  Diag << FixItHint::CreateReplacement(
    CharSourceRange::getTokenRange(SourceRange(EqualLoc, InitRange.getEnd())),
    Replacement
  );
}

void InitCheck::checkUnsignedSuffix(const VarDecl* VD, ASTContext* Ctx) {
  Expr* Init = VD->getInit();
  if (!Init) return;

  IntegerLiteral* Lit = dyn_cast<IntegerLiteral>(Init->IgnoreParenImpCasts());
  if (!Lit) return;

  // Check if suffix already present
  SourceManager& SM = Ctx->getSourceManager();
  StringRef Text = Lexer::getSourceText(
    CharSourceRange::getTokenRange(Lit->getSourceRange()),
    SM, Ctx->getLangOpts()
  );

  if (Text.endswith_insensitive("u") || Text.endswith_insensitive("U")) {
    return; // Already has suffix
  }

  auto Diag = diag(Lit->getLocation(),
    "unsigned integer literal without 'U' suffix");

  // Fix: Add U suffix
  Diag << FixItHint::CreateInsertionAfter(Lit->getEndLoc(), "U");
}

// Skip predicates (implementations)
bool InitCheck::shouldSkipAuto(const VarDecl* VD) {
  return VD->getType()->isAutoType();
}

bool InitCheck::shouldSkipForLoop(const VarDecl* VD) {
  // Check if in for-range or traditional for loop init
  return VD->getDeclContext()->getParent()->isFunctionOrMethod();
}

bool InitCheck::shouldSkipUnion(const VarDecl* VD) {
  if (const auto* RD = dyn_cast<RecordDecl>(VD->getDeclContext())) {
    return RD->isUnion();
  }
  return false;
}

bool InitCheck::shouldSkipExtern(const VarDecl* VD) {
  return VD->hasExternalStorage() && !VD->hasInit();
}

bool InitCheck::shouldSkipException(const VarDecl* VD) {
  // Check if in catch statement
  return false; // Simplified for matcher version
}

} // namespace codelint
} // namespace clang::tidy
```

#### 2.4 Parallel Execution Wave

```
┌─────────────────────────────────────────────────────────┐
│               PARALLEL CHECKER MIGRATION                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Wave 1 (SIMULTANEOUS - 3 agents):                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │ GlobalCheck │  │SingletonChk │  │  InitCheck  │     │
│  │  (1-2 hrs)  │  │  (1-2 hrs)  │  │  (4-6 hrs)  │     │
│  │  LOW RISK   │  │  LOW RISK   │  │  HIGH RISK  │     │
│  └─────────────┘  └─────────────┘  └─────────────┘     │
│                                                         │
│  Dependency: None (can start independently)            │
│                                                         │
│  Total: 6-10 hours with 3 parallel agents              │
│  Sequential: 12-16 hours                               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**Tasks**:

| Task ID | Description | Category | Skill | Dependencies | Duration |
|---------|-------------|----------|-------|--------------|----------|
| P2-T1 | GlobalCheck matcher design | implementation | ast-matchers | P1 | 30m |
| P2-T2 | GlobalCheck implementation | implementation | clang-tidy | P2-T1 | 1h |
| P2-T3 | GlobalCheck test cases | testing | gtest | P2-T2 | 30m |
| P2-T4 | SingletonCheck matcher | implementation | ast-matchers | P1 | 30m |
| P2-T5 | SingletonCheck implementation | implementation | clang-tidy | P2-T4 | 1h |
| P2-T6 | SingletonCheck test cases | testing | gtest | P2-T5 | 30m |
| P2-T7 | InitCheck matcher design | implementation | ast-matchers | P1 | 1h |
| P2-T8 | InitCheck implementation | implementation | clang-tidy | P2-T7 | 3h |
| P2-T9 | InitCheck FixIt hints | implementation | clang-tidy | P2-T8 | 1h |
| P2-T10 | InitCheck test cases | testing | gtest | P2-T9 | 1h |

---

### Phase 3: Testing Migration

**Duration**: 4-6 hours
**Dependencies**: Phase 2 complete

#### 3.1 Test Infrastructure

**ClangTidyTest Harness**:

```cpp
// tests/check-tests/init-check-test.cpp

#include "gtest/gtest.h"
#include "clang-tidy/ClangTidyTest.h"
#include "codelint/checks/InitCheck.h"

using namespace clang::tidy::test;

namespace clang::tidy {
namespace codelint {
namespace test {

TEST(InitCheckTest, UninitializedVariable) {
  std::string Code = "void f() { int x; }";
  std::string Expected = "void f() { int x{}; }";

  EXPECT_EQ(Expected, runCheckOnCode<InitCheck>(Code));
}

TEST(InitCheckTest, EqualsInitToBraceInit) {
  std::string Code = "void f() { int x = 5; }";
  std::string Expected = "void f() { int x{5}; }";

  EXPECT_EQ(Expected, runCheckOnCode<InitCheck>(Code));
}

TEST(InitCheckTest, UnsignedSuffix) {
  std::string Code = "void f() { unsigned int x = 42; }";
  std::string Expected = "void f() { unsigned int x = 42U; }";

  EXPECT_EQ(Expected, runCheckOnCode<InitCheck>(Code));
}

TEST(InitCheckTest, AutoNotReported) {
  std::string Code = "void f() { auto x = 5; }";

  // Should NOT report anything
  EXPECT_EQ(Code, runCheckOnCode<InitCheck>(Code));
}

} // namespace test
} // namespace codelint
} // namespace clang::tidy
```

#### 3.2 Golden File Migration

**Strategy**: Convert existing test fixtures to clang-tidy format

```
Current: tests/CodeLintTest/src/init_checker/src/*.cpp
Target: tests/check-tests/cases/init-check/*.cpp

Conversion Process:
1. Copy test cases
2. Add // CHECK-NOTES: annotations (clang-tidy format)
3. Update test runner to use clang-tidy test framework
```

**Example Test File**:

```cpp
// tests/check-tests/cases/init-check/uninitialized.cpp

void test_uninitialized() {
  int x;  // CHECK-NOTES: [[@LINE]]:7: warning: uninitialized variable 'x'
          // CHECK-NOTES: [[@LINE]]:7: note: fix suggested: "int x{}"
}

void test_initialized() {
  int y{};  // CHECK-NOTES-NOT: warning
}
```

#### 3.3 Test Migration Tasks

| Task ID | Description | Category | Skill | Dependencies | Duration |
|---------|-------------|----------|-------|--------------|----------|
| P3-T1 | Setup ClangTidyTest harness | testing | gtest | P2 | 1h |
| P3-T2 | Port InitCheck tests | testing | clang-tidy | P3-T1 | 1h |
| P3-T3 | Port GlobalCheck tests | testing | clang-tidy | P3-T1 | 30m |
| P3-T4 | Port SingletonCheck tests | testing | clang-tidy | P3-T1 | 30m |
| P3-T5 | Convert golden files | testing | file-ops | P3-T2-T4 | 1h |
| P3-T6 | Integration test | testing | bash | P3-T5 | 30m |
| P3-T7 | Update CMake test config | build | cmake | P3-T6 | 30m |

---

### Phase 4: Documentation

**Duration**: 2-3 hours
**Dependencies**: Phase 3 complete

#### 4.1 Check Documentation Format

```markdown
# docs/check-docs/codelint-init.md

# codelint-init

Checks for proper variable initialization style in C++ code.

## Description

This check enforces modern C++ initialization best practices:

1. **Uninitialized variables**: Variables must be explicitly initialized
2. **Brace initialization**: Prefer `{}` over `=` syntax
3. **Unsigned suffix**: Add `U` suffix to unsigned integer literals

## Options

None (all checks enabled by default)

## Examples

### Example 1: Uninitialized variable

```cpp
// Before
void f() {
  int x;
}

// After
void f() {
  int x{};
}
```

### Example 2: Equals to brace init

```cpp
// Before
int value = 42;

// After
int value{42};
```

### Example 3: Unsigned suffix

```cpp
// Before
unsigned int count = 5;

// After
unsigned int count = 5U;
```

## Limitations

- Does not detect const/constexpr opportunities (requires CFG analysis)
- For loop variables are intentionally skipped
- Auto declarations require `=` syntax (not flagged)

## See Also

- [cppcoreguidelines-pro-type-member-init](https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/pro-type-member-init.html)
- [modernize-use-auto](https://clang.llvm.org/extra/clang-tidy/checks/modernize/use-auto.html)
```

#### 4.2 Integration Guide

```markdown
# docs/clang-tidy-integration.md

# Using codelint as a clang-tidy Plugin

## Installation

### Build the plugin

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Install to clang-tidy

```bash
# Copy plugin to clang-tidy library directory
sudo cp build/codelint-plugin.so /usr/lib/clang-tidy/
```

## Usage

### Load plugin and run checks

```bash
# Load plugin
clang-tidy --load=/usr/lib/clang-tidy/codelint-plugin.so \
           --checks='codelint-*' \
           main.cpp

# With compilation database
clang-tidy --load=codelint-plugin.so \
           --checks='codelinit-*' \
           -p build \
           src/**/*.cpp

# Apply fixes
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
CheckOptions:
  - key: codelint-init.StrictMode
    value: true
```

### Selective checks

```bash
# Only initialization checks
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-init' \
           main.cpp

# Only global/singleton
clang-tidy --load=codelint-plugin.so \
           --checks='codelint-global, codelint-singleton' \
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
clang-tidy-diff.py --clang-tidy-binary clang-tidy \
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
        run: cmake -B build && cmake --build build
      - name: Run codelint
        run: clang-tidy --load=build/codelint-plugin.so \
                        --checks='codelint-*' \
                        -p build \
                        src/**/*.cpp
```

## Differences from Standalone codelint

| Feature | Standalone | Plugin |
|---------|------------|--------|
| CLI | Custom CLI11 | clang-tidy CLI |
| Git scope | Built-in | ❌ Deleted |
| SARIF output | Built-in | clang-tidy native |
| Fixes | String-based | FixItHint |
| Const suggestions | CFG-based | ❌ Removed |
| Configuration | Custom | .clang-tidy file |
```

#### 4.3 Documentation Tasks

| Task ID | Description | Category | Skill | Dependencies | Duration |
|---------|-------------|----------|-------|--------------|----------|
| P4-T1 | InitCheck documentation | docs | markdown | P3 | 30m |
| P4-T2 | GlobalCheck documentation | docs | markdown | P3 | 15m |
| P4-T3 | SingletonCheck documentation | docs | markdown | P3 | 15m |
| P4-T4 | Integration guide | docs | markdown | P4-T1-T3 | 1h |
| P4-T5 | README update | docs | markdown | P4-T4 | 30m |
| P4-T6 | Example .clang-tidy | docs | yaml | P4-T4 | 15m |

---

### Phase 5: Cleanup

**Duration**: 2-3 hours
**Dependencies**: All phases complete

#### 5.1 Files to Delete

```bash
# DELETE old CLI infrastructure
rm -f src/main.cpp
rm -f src/commands/cmd_check_init.cpp
rm -f src/commands/cmd_find_global.cpp
rm -f src/commands/cmd_find_singleton.cpp
rm -f src/commands/cmd_utils.cpp
rm -f include/commands/cmd_check_init.h
rm -f include/commands/cmd_find_global.h
rm -f include/commands/cmd_find_singleton.h
rm -f include/commands/cmd_utils.h
rm -f include/commands.h
rm -f include/lint_commands.h

# DELETE old checker infrastructure
rm -f src/lint_checker.cpp
rm -f src/lint/lint_runner.cpp
rm -f src/lint/lint_action.cpp
rm -f src/lint_visitor.cpp
rm -f src/lint.cpp
rm -f include/lint/lint_checker.h
rm -f include/lint/lint_runner.h
rm -f include/lint/lint_action.h
rm -f include/lint/lint_visitor.h
rm -f include/lint/lint_types.h

# DELETE old checkers (migrated)
rm -f src/lint/checkers/init_checker.cpp
rm -f src/lint/checkers/global_checker.cpp
rm -f src/lint/checkers/singleton_checker.cpp
rm -f include/lint/checkers/init_checker.h
rm -f include/lint/checkers/global_checker.h
rm -f include/lint/checkers/singleton_checker.h

# DELETE output/reporting infrastructure
rm -f src/issue_reporter.cpp
rm -f src/fix_applier.cpp
rm -f src/lint/git_scope.cpp
rm -f include/lint/issue_reporter.h
rm -f include/lint/fix_applier.h
rm -f include/lint/git_scope.h

# DELETE old tests (migrated)
rm -f tests/integration_test.cpp
rm -f tests/commands_test.cpp
rm -f tests/sarif_validation_test.cpp
rm -f tests/text_format_test.cpp
rm -f tests/backward_compat_test.cpp

# DELETE old CMake
rm -f CMakeLists.txt
mv CMakeLists.txt.clangtidy CMakeLists.txt
```

#### 5.2 Dependencies to Remove

```cmake
# Remove from CMakeLists.txt:
# - find_package(libgit2 REQUIRED)
# - find_path(RAPIDJSON_INCLUDE_DIR ...)
# - CLI11 (embedded in source)
```

#### 5.3 Cleanup Tasks

| Task ID | Description | Category | Skill | Dependencies | Duration |
|---------|-------------|----------|-------|--------------|----------|
| P5-T1 | Delete CLI files | cleanup | file-ops | P4 | 10m |
| P5-T2 | Delete checker infrastructure | cleanup | file-ops | P4 | 10m |
| P5-T3 | Delete output/reporting | cleanup | file-ops | P4 | 10m |
| P5-T4 | Delete old tests | cleanup | file-ops | P4 | 10m |
| P5-T5 | Remove dependencies from CMake | cleanup | cmake | P5-T1-T4 | 15m |
| P5-T6 | Final build verification | verification | build | P5-T5 | 30m |
| P5-T7 | Update README | docs | markdown | P5-T6 | 30m |

---

## 4. Risk Assessment

### 4.1 Risk Matrix

| Risk | Severity | Probability | Mitigation |
|------|----------|-------------|------------|
| InitCheck CFG complexity | HIGH | HIGH | Document limitation, manual flow analysis |
| Plugin loading failure | MEDIUM | LOW | Comprehensive testing, platform-specific handling |
| Matcher false positives | MEDIUM | MEDIUM | Thorough skip predicates, test coverage |
| Missing features (const suggestions) | HIGH | CERTAIN | Document clearly, provide alternative (cppcoreguidelines) |
| Build system compatibility | MEDIUM | MEDIUM | Cross-platform CMake, LLVM version handling |
| Test framework incompatibility | LOW | LOW | ClangTidyTest is standard |

### 4.2 Critical Path

```
┌─────────────────────────────────────────────────────────┐
│                    CRITICAL PATH                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  P1 (Infrastructure)                                    │
│      │                                                  │
│      ▼                                                  │
│  P2-T7 (InitCheck matcher design) ── CRITICAL          │
│      │                                                  │
│      ▼                                                  │
│  P2-T8 (InitCheck implementation) ── CRITICAL          │
│      │                                                  │
│      ▼                                                  │
│  P3-T2 (InitCheck tests)                               │
│      │                                                  │
│      ▼                                                  │
│  P5 (Cleanup)                                          │
│                                                         │
│  Total Critical Path: 10-12 hours                      │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 4.3 Feature Gaps

| Current Feature | Target Status | Alternative |
|-----------------|---------------|-------------|
| const suggestions | ❌ REMOVED | Use `cppcoreguidelines-macro-constants` |
| constexpr suggestions | ❌ REMOVED | Manual review + `readability-identifier-naming` |
| Git scope filtering | ❌ REMOVED | Use clang-tidy-diff.py for CI |
| SARIF output | ❌ REMOVED | clang-tidy-to-sarif.py tool |
| Custom text output | ❌ REMOVED | clang-tidy default console output |
| suppress_constant | ❌ REMOVED | Use .clang-tidy CheckOptions |

---

## 5. Timeline Estimate

### 5.1 Sequential Timeline

```
Week 1:
├─ Day 1-2: Phase 1 (Infrastructure Setup) ───────────── 6h
├─ Day 3-4: Phase 2 (Checker Rewriting)
│   ├─ GlobalCheck ──────────────────────────────────── 2h
│   ├─ SingletonCheck ───────────────────────────────── 2h
│   └─ InitCheck ────────────────────────────────────── 6h
├─ Day 5: Phase 3 (Testing Migration) ────────────────── 5h

Week 2:
├─ Day 1: Phase 4 (Documentation) ────────────────────── 3h
├─ Day 2: Phase 5 (Cleanup) ──────────────────────────── 3h
├─ Day 3: Buffer for issues ──────────────────────────── 4h

Total: 30-35 hours over 2 weeks
```

### 5.2 Parallel Timeline (with 3 agents)

```
Day 1:
├─ Agent 1: P1-T1 (Directory structure) ─────────────── 15m
├─ Agent 1: P1-T2 (CodelintModule) ──────────────────── 30m
├─ Agent 2: P1-T3 (CMake config) ────────────────────── 30m
├─ Agent 1: P1-T5 (Test loading) ────────────────────── 30m
└─ Total: 2 hours (with coordination)

Day 2 (PARALLEL):
├─ Agent 1: P2-T1-T3 (GlobalCheck) ──────────────────── 2h
├─ Agent 2: P2-T4-T6 (SingletonCheck) ───────────────── 2h
├─ Agent 3: P2-T7-T9 (InitCheck) ────────────────────── 5h
└─ Total: 5 hours (parallel execution)

Day 3:
├─ All Agents: P3 (Testing Migration) ───────────────── 5h

Day 4:
├─ Agent 1: P4 (Documentation) ──────────────────────── 3h
├─ Agent 2: P5 (Cleanup) ────────────────────────────── 3h

Total: 15-20 hours with 3 parallel agents over 4 days
```

---

## 6. Deliverables Checklist

### Phase 1 Deliverables
- [ ] Plugin directory structure created
- [ ] CodelintModule.cpp/h implemented
- [ ] CMake configuration for plugin build
- [ ] Plugin loads successfully in clang-tidy
- [ ] .clang-tidy example configuration file

### Phase 2 Deliverables
- [ ] GlobalCheck implemented with AST Matchers
- [ ] SingletonCheck implemented with AST Matchers
- [ ] InitCheck implemented with AST Matchers
- [ ] All checks produce FixItHints
- [ ] Checks visible via `--list-checks`

### Phase 3 Deliverables
- [ ] ClangTidyTest harness configured
- [ ] InitCheck unit tests passing
- [ ] GlobalCheck unit tests passing
- [ ] SingletonCheck unit tests passing
- [ ] Golden file tests converted

### Phase 4 Deliverables
- [ ] Check documentation (3 markdown files)
- [ ] Integration guide (markdown)
- [ ] README updated for plugin usage
- [ ] Example .clang-tidy configuration

### Phase 5 Deliverables
- [ ] All old files deleted
- [ ] Dependencies removed (CLI11, libgit2, RapidJSON)
- [ ] Build succeeds with new CMake
- [ ] No orphaned references in codebase
- [ ] Final verification complete

---

## 7. Implementation Notes

### 7.1 Key Architecture Decisions

1. **Check Naming**: Use `codelint-*` prefix for clarity
2. **Matcher Approach**: Declarative patterns over manual traversal
3. **Fix Application**: FixItHints for inline fixes
4. **Configuration**: Standard .clang-tidy format
5. **Output**: clang-tidy native formats only

### 7.2 Const/Constexpr Handling

**Decision**: Remove const/constexpr suggestions

**Rationale**:
- Requires CFG-based modification tracking
- AST Matchers are pattern-based, not flow-based
- clang's dataflow analysis framework may not be accessible in clang-tidy context
- Alternative: Recommend users use `cppcoreguidelines` checks

**Alternative for Users**:
```bash
# Use cppcoreguidelines for const/constexpr detection
clang-tidy --checks='codelint-*,
                     cppcoreguidelines-avoid-magic-values,
                     readability-identifier-naming' \
           main.cpp
```

### 7.3 Git Scope Replacement

**Decision**: Delete git scope feature

**Alternative**: Use clang-tidy-diff.py

```bash
# Check only changed lines
git diff -U0 HEAD^ | clang-tidy-diff.py \
  --clang-tidy-binary clang-tidy \
  --load codelint-plugin.so \
  --checks 'codelint-*'
```

---

## 8. Conclusion

This refactoring transforms codelint from a standalone binary to a clang-tidy plugin, simplifying the architecture and leveraging clang-tidy's built-in infrastructure. The migration preserves core detection capabilities while removing custom features as mandated by user decisions.

**Key Benefits**:
- Reduced code complexity (estimate: -2000 lines)
- Standard clang-tidy integration
- Built-in output formats and fix mechanism
- No dependency maintenance (CLI11, libgit2, RapidJSON)
- Leverage clang-tidy's extensive test infrastructure

**Trade-offs**:
- Lost features: Git scope, const/constexpr suggestions, custom output
- Alternative solutions documented for users
- AST Matcher limitations for flow-sensitive analysis

**Success Metrics**:
- Plugin compiles and loads in clang-tidy
- All 3 checks detect expected issues
- Tests pass with new harness
- Documentation complete and accurate
- Build clean with no orphaned references
