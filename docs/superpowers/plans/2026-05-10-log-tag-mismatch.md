# Log Tag Mismatch Checker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a clang-tidy check that detects mismatched log tags (`[FuncName]`) in logging statements, providing warnings and automatic FixIt corrections.

**Architecture:** Follow existing codelint checker pattern. Use AST matchers to find log calls, extract string literals, validate tags against enclosing function name.

**Tech Stack:** Clang AST Matchers, Clang-Tidy framework, C++, Regex, LIT testing framework

---

## Pre-requisite: Verify Build Environment

**Files:**
- Check: `CMakeLists.txt`, `tests/lit.cfg.in`

- [ ] **Step 1: Build and run existing tests to confirm baseline**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
cmake --build cmake-build-debug --target check-codelint-init -j8
```

Expected: All tests pass

---

## Task 1: Create Checker Header File

**Files:**
- Create: `include/codelint/checks/LogTagMismatchCheck.h`

- [ ] **Step 1: Write the header file**

```cpp
#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <vector>
#include <string>

namespace clang::tidy::codelint {

class LogTagMismatchCheck : public ClangTidyCheck {
public:
  LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context);

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;
  void storeOptions(ClangTidyOptions::OptionMap& Opts) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  // Configuration options
  std::string LogMacroNames;
  bool AllowQualifiedName;

  // Helper methods
  const FunctionDecl* getEnclosingFunction(const Stmt* S, ASTContext* Ctx);
  std::vector<std::string> extractTags(StringRef LogText);
  bool isTagValid(StringRef Tag, const FunctionDecl* Func);
};

} // namespace clang::tidy::codelint
```

- [ ] **Step 2: Verify file compiles**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds (header only, no implementation yet)

---

## Task 2: Register Checker in Module

**Files:**
- Modify: `src/codelint_plugin/CodelintModule.cpp`

- [ ] **Step 1: Add include and registration**

```cpp
// Add include (after line 11)
#include "codelint/checks/LogTagMismatchCheck.h"

// Add in addCheckFactories (before line 30)
CheckFactories.registerCheck<LogTagMismatchCheck>("codelint-log-tag-mismatch");
```

- [ ] **Step 2: Verify build still works**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Linker error about undefined LogTagMismatchCheck (normal for now)

---

## Task 3: Create First Lit Test - Basic Mismatch Detection

**Files:**
- Create: `tests/lit_style_tests_v2/log_tag_mismatch_checker/basic_mismatch.cpp`

- [ ] **Step 1: Create test directory and file**

```bash
mkdir -p tests/lit_style_tests_v2/log_tag_mismatch_checker
```

- [ ] **Step 2: Write failing test**

```cpp
// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for basic log tag mismatch detection

// Mock log macro
#define LOG(msg) printf(msg)

void FuncA() {
    LOG("[FuncA] correct tag");
    // No warning expected - tag matches function

    LOG("[FuncB] wrong tag");
    // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: log tag 'FuncB' does not match enclosing function 'FuncA'  [codelint-log-tag-mismatch]
}

void FuncB() {
    LOG("[FuncB] correct again");
}
```

- [ ] **Step 3: Run test to verify it fails**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: Test fails - checker not implemented yet

---

## Task 4: Implement Basic Checker Skeleton

**Files:**
- Create: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Create minimal implementation**

```cpp
#include "codelint/checks/LogTagMismatchCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <regex>

namespace clang::tidy::codelint {

using namespace ast_matchers;

LogTagMismatchCheck::LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context)
    : ClangTidyCheck(Name, Context),
      LogMacroNames(Options.get("LogMacroNames", "*LOG*,*log*")),
      AllowQualifiedName(Options.get("AllowQualifiedName", true)) {}

void LogTagMismatchCheck::storeOptions(ClangTidyOptions::OptionMap& Opts) {
  Options.store(Opts, "LogMacroNames", LogMacroNames);
  Options.store(Opts, "AllowQualifiedName", AllowQualifiedName);
}

void LogTagMismatchCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  // Match call expressions - we'll filter by macro name in check()
  Finder->addMatcher(callExpr().bind("logCall"), this);
}

const FunctionDecl* LogTagMismatchCheck::getEnclosingFunction(const Stmt* S, ASTContext* Ctx) {
  // TODO: Implement traversal to find enclosing function
  return nullptr;
}

std::vector<std::string> LogTagMismatchCheck::extractTags(StringRef LogText) {
  std::vector<std::string> Tags;
  // TODO: Extract [Name] patterns
  return Tags;
}

bool LogTagMismatchCheck::isTagValid(StringRef Tag, const FunctionDecl* Func) {
  // TODO: Validate tag against function name
  return true;
}

void LogTagMismatchCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  const auto* Call = Result.Nodes.getNodeAs<CallExpr>("logCall");
  if (Call == nullptr) {
    return;
  }

  // TODO: Implement actual checking logic
}

} // namespace clang::tidy::codelint
```

- [ ] **Step 2: Build to verify skeleton compiles**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds

---

## Task 5: Implement getEnclosingFunction Helper

**Files:**
- Modify: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Add include for AST traversal**

```cpp
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
```

- [ ] **Step 2: Implement getEnclosingFunction**

```cpp
const FunctionDecl* LogTagMismatchCheck::getEnclosingFunction(const Stmt* S, ASTContext* Ctx) {
  // Traverse up the parent chain to find the enclosing function
  const Stmt* Current = S;
  const FunctionDecl* FoundFunc = nullptr;

  while (Current) {
    // Get the parent of Current statement
    auto Parents = Ctx->getParents(*Current);
    if (Parents.empty()) {
      break;
    }

    const auto* ParentNode = Parents.begin();
    if (const auto* FD = ParentNode->get<FunctionDecl>()) {
      FoundFunc = FD;
      break;
    }
    if (const auto* MD = ParentNode->get<CXXMethodDecl>()) {
      FoundFunc = MD;
      break;
    }
    if (const auto* LS = ParentNode->get<LambdaExpr>()) {
      // For lambdas, continue to find the outer function
      Current = *Parents.begin()->get<Stmt>();
      continue;
    }
    if (const auto* ParentStmt = ParentNode->get<Stmt>()) {
      Current = ParentStmt;
    } else {
      break;
    }
  }

  return FoundFunc;
}
```

- [ ] **Step 3: Build and verify**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds

---

## Task 6: Implement extractTags Helper

**Files:**
- Modify: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Implement extractTags with regex**

```cpp
std::vector<std::string> LogTagMismatchCheck::extractTags(StringRef LogText) {
  std::vector<std::string> Tags;
  std::regex TagPattern(R"(\[([A-Za-z0-9_:]+)\])");
  std::string Text = LogText.str();

  auto WordsBegin = std::sregex_iterator(Text.begin(), Text.end(), TagPattern);
  auto WordsEnd = std::sregex_iterator();

  for (std::sregex_iterator I = WordsBegin; I != WordsEnd; ++I) {
    std::smatch Match = *I;
    Tags.push_back(Match[1].str());
  }

  return Tags;
}
```

- [ ] **Step 2: Build and verify**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds

---

## Task 7: Implement isTagValid Helper

**Files:**
- Modify: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Implement isTagValid**

```cpp
bool LogTagMismatchCheck::isTagValid(StringRef Tag, const FunctionDecl* Func) {
  if (!Func) {
    return true;  // Can't validate, assume valid
  }

  std::string FuncName = Func->getNameAsString();

  // Check direct match
  if (Tag == FuncName) {
    return true;
  }

  // Check qualified name match if enabled
  if (AllowQualifiedName) {
    if (const auto* MD = dyn_cast<CXXMethodDecl>(Func)) {
      std::string QualifiedName = MD->getParent()->getNameAsString() + "::" + FuncName;
      if (Tag == QualifiedName) {
        return true;
      }
    }
  }

  return false;
}
```

- [ ] **Step 2: Build and verify**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds

---

## Task 8: Implement Main check() Logic

**Files:**
- Modify: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Implement check() method**

```cpp
void LogTagMismatchCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  const auto* Call = Result.Nodes.getNodeAs<CallExpr>("logCall");
  if (Call == nullptr) {
    return;
  }

  // Check if this call is from a log macro
  SourceLocation CallLoc = Call->getBeginLoc();
  if (!CallLoc.isMacroID()) {
    return;  // Not a macro call
  }

  SourceManager& SM = Result.Context->getSourceManager();
  StringRef MacroName = Lexer::getImmediateMacroName(CallLoc, SM, Result.Context->getLangOpts());

  // TODO: Use pattern matching instead of exact contains
  if (!MacroName.contains("LOG") && !MacroName.contains("log")) {
    return;
  }

  // Get enclosing function
  const FunctionDecl* EnclosingFunc = getEnclosingFunction(Call, Result.Context);
  if (!EnclosingFunc) {
    return;
  }

  std::string FuncName = EnclosingFunc->getNameAsString();

  // Check all arguments for string literals
  for (unsigned I = 0; I < Call->getNumArgs(); ++I) {
    const Expr* Arg = Call->getArg(I)->IgnoreImplicit();
    if (const auto* SL = dyn_cast<StringLiteral>(Arg)) {
      std::string LogText = SL->getString().str();
      std::vector<std::string> Tags = extractTags(LogText);

      for (const auto& Tag : Tags) {
        if (!isTagValid(Tag, EnclosingFunc)) {
          // Find location of the tag in the source
          SourceLocation TagLoc = SL->getBeginLoc();
          size_t TagPos = LogText.find("[" + Tag + "]");
          if (TagPos != std::string::npos) {
            TagLoc = TagLoc.getLocWithOffset(TagPos);
          }

          diag(TagLoc, "log tag '%0' does not match enclosing function '%1'")
              << Tag << FuncName
              << FixItHint::CreateReplacement(
                  SourceRange(TagLoc, TagLoc.getLocWithOffset(Tag.size() + 1)),
                  "[" + FuncName + "]");
        }
      }
    }
  }
}
```

- [ ] **Step 2: Add missing include**

```cpp
#include <clang/Lex/Lexer.h>
```

- [ ] **Step 3: Build**

```bash
cmake --build cmake-build-debug --target codelint-plugin -j8
```

Expected: Build succeeds

- [ ] **Step 4: Run first test**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: Test now passes

---

## Task 9: Add Test for "No Tag" Case

**Files:**
- Create: `tests/lit_style_tests_v2/log_tag_mismatch_checker/no_tag.cpp`

- [ ] **Step 1: Write test**

```cpp
// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test that logs without tags don't trigger warnings

#define LOG(msg) printf(msg)

void FuncA() {
    LOG("no tag at all");
    LOG("just some message");
    LOG("brackets [but not identifier]");
    LOG("[123] numeric only");
}
```

- [ ] **Step 2: Run test**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: Test passes - no warnings for untagged logs

- [ ] **Step 3: Fix checker if needed to handle edge cases**

Adjust extractTags regex if needed to only match valid identifier patterns

---

## Task 10: Add Test for Qualified Name Support

**Files:**
- Create: `tests/lit_style_tests_v2/log_tag_mismatch_checker/qualified_name.cpp`

- [ ] **Step 1: Write test**

```cpp
// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for qualified name tags [Class::Method]

#define LOG(msg) printf(msg)

class MyClass {
public:
    void FuncA() {
        LOG("[FuncA] simple match");
        LOG("[MyClass::FuncA] qualified match");

        LOG("[OtherClass::FuncA] wrong class");
        // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'OtherClass::FuncA' does not match enclosing function 'FuncA'

        LOG("[MyClass::WrongFunc] wrong method");
        // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'MyClass::WrongFunc' does not match enclosing function 'FuncA'
    }
};
```

- [ ] **Step 2: Run test**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: Test passes

---

## Task 11: Add Test for Lambda Support

**Files:**
- Create: `tests/lit_style_tests_v2/log_tag_mismatch_checker/lambda_support.cpp`

- [ ] **Step 1: Write test**

```cpp
// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test that lambdas allow outer function name as tag

#define LOG(msg) printf(msg)

void OuterFunc() {
    auto lambda1 = []() {
        LOG("[OuterFunc] inside lambda");  // Should be valid - matches outer function
    };

    auto lambda2 = []() {
        LOG("[WrongFunc] wrong tag");
        // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'WrongFunc' does not match enclosing function 'OuterFunc'
    };
}
```

- [ ] **Step 2: Run test**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: Test passes (fix getEnclosingFunction if needed)

---

## Task 12: Implement Macro Name Pattern Matching

**Files:**
- Modify: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`

- [ ] **Step 1: Add helper for pattern matching**

```cpp
// Add to private section:
bool matchesMacroPattern(StringRef MacroName);

// Implement the helper:
bool LogTagMismatchCheck::matchesMacroPattern(StringRef MacroName) {
  // Split LogMacroNames by commas and check each pattern
  std::string Patterns = LogMacroNames;
  size_t Pos = 0;
  while (Pos < Patterns.size()) {
    size_t Next = Patterns.find(',', Pos);
    std::string Pattern;
    if (Next == std::string::npos) {
      Pattern = Patterns.substr(Pos);
      Next = Patterns.size();
    } else {
      Pattern = Patterns.substr(Pos, Next - Pos);
    }
    Pos = Next + 1;

    // Simple glob matching: * = any chars
    // Convert to lowercase for case-insensitive match
    std::string MacroLower = MacroName.lower();
    std::string PatternLower;
    std::transform(Pattern.begin(), Pattern.end(), std::back_inserter(PatternLower),
                   [](unsigned char c) { return std::tolower(c); });

    // Simple pattern matching
    // For now, just check if pattern (without *) is contained
    // TODO: Implement proper glob matching
    std::string PatternStripped;
    for (char c : PatternLower) {
      if (c != '*') {
        PatternStripped += c;
      }
    }
    if (MacroLower.find(PatternStripped) != std::string::npos) {
      return true;
    }
  }
  return false;
}
```

- [ ] **Step 2: Update check() to use pattern matching**

Replace the hardcoded check:
```cpp
// Replace:
// if (!MacroName.contains("LOG") && !MacroName.contains("log")) {

// With:
if (!matchesMacroPattern(MacroName)) {
    return;
}
```

- [ ] **Step 3: Add test for custom macro names**

Create `tests/lit_style_tests_v2/log_tag_mismatch_checker/custom_macros.cpp`:

```cpp
// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for various log macro naming patterns

#define INFO_LOG(msg) printf(msg)
#define debug_log(msg) printf(msg)
#define WARN_MSG(msg) printf(msg)

void FuncA() {
    INFO_LOG("[FuncB] wrong");
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: log tag 'FuncB'

    debug_log("[FuncB] wrong");
    // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: log tag 'FuncB'

    WARN_MSG("[FuncB] wrong");
    // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'FuncB'
}
```

- [ ] **Step 4: Run all tests**

```bash
cmake --build cmake-build-debug --target check-codelint-log-tag-mismatch -j8
```

Expected: All tests pass

---

## Task 13: Run Full Test Suite and Fix Issues

**Files:** All test and source files

- [ ] **Step 1: Run all codelint tests**

```bash
cmake --build cmake-build-debug --target check-codelint -j8
```

- [ ] **Step 2: Fix any regressions**

Expected: All tests pass

- [ ] **Step 3: Run clang-format on new files**

```bash
clang-format -i include/codelint/checks/LogTagMismatchCheck.h
clang-format -i src/codelint_plugin/checks/LogTagMismatchCheck.cpp
```

---

## Task 14: Add README/Documentation

**Files:**
- Create: `docs/log_tag_mismatch_checker.md`

- [ ] **Step 1: Write documentation**

```markdown
# Log Tag Mismatch Checker

## What it does

Checks that log tags (`[FunctionName]`) in logging statements match the name of the
enclosing function. This helps catch copy-paste errors when moving log statements
between functions.

## Example

```cpp
void Foo::FuncA() {
    ILOG("[FuncB] enters");     // Warning: log tag 'FuncB' does not match enclosing function 'FuncA'
    ILOG("[FuncA] finished");   // OK
}
```

## Options

| Name | Default | Description |
|------|---------|-------------|
| `LogMacroNames` | `"*LOG*,*log*"` | Comma-separated patterns for log macro names |
| `AllowQualifiedName` | `true` | Allow `[Class::Func]` format for member functions |

## Supported Patterns

- `[FuncName]` - Simple function name match
- `[Class::FuncName]` - Qualified name match (when enabled)
- No tag - No warning (logs without tags are allowed)
```

---

## Final Verification

- [ ] All tests pass
- [ ] Code follows project style
- [ ] Checker registered correctly
- [ ] Documentation complete
