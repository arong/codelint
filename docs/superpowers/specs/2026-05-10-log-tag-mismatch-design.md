---
name: Log Tag Mismatch Checker Design
description: Design document for codelint-log-tag-mismatch checker
type: reference
---

# Log Tag Mismatch Checker Design

## Overview

A clang-tidy check that verifies log headers (tags) in logging statements match the
enclosing function name. This helps catch copy-paste errors where developers forget
to update the log tag when copying log statements between functions.

## Problem Statement

Developers often add log tags like `[FuncName]` to logging statements to identify
which function generated the log. When copying log statements between functions,
it's common to forget updating the tag, leading to misleading log output.

Example:
```cpp
void Foo::FuncA() {
    ILOG("[FuncB] enters");     // ERROR: copied from FuncB, tag not updated
    ILOG("[FuncA] finished");   // OK
}
```

## Requirements

### Core Functionality
1. Detect logging macro calls in source code
2. Extract all `[Name]` patterns from log string literals
3. Get the enclosing function name where the log is located
4. Warn if any `[Name]` tag does not match the enclosing function
5. Provide FixIt hint to automatically correct the tag

### Edge Cases
1. **No tag present**: If there's no `[Name]` pattern in the log, do NOT warn
2. **Lambda functions**: Allow the outer function name as valid tag
3. **Qualified names**: Allow `[Class::Func]` as well as `[Func]` (configurable)
4. **Multiple tags**: If multiple `[Name]` patterns exist, warn only on mismatches

## Configuration Options

| Option | Type | Default Value | Description |
|--------|------|---------------|-------------|
| `LogMacroNames` | string | `"*LOG*,*log*"` | Comma-separated logging macro name patterns, supports glob wildcards |
| `AllowQualifiedName` | bool | `true` | Allow `[Class::Function]` format in addition to `[Function]` |

### clang-tidy Option Registration

```cpp
LogTagMismatchCheck::LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context)
    : ClangTidyCheck(Name, Context),
      LogMacroNames(Options.get("LogMacroNames", "*LOG*,*log*")),
      AllowQualifiedName(Options.get("AllowQualifiedName", true)) {}

void LogTagMismatchCheck::storeOptions(ClangTidyOptions::OptionMap& Opts) {
  Options.store(Opts, "LogMacroNames", LogMacroNames);
  Options.store(Opts, "AllowQualifiedName", AllowQualifiedName);
}
```

## Implementation Approach

### AST Matching Strategy

Logging calls are typically implemented as macros. We need to handle both:
1. Direct function calls to logging functions
2. Macro expansions that result in function calls

Approach:
- Use `isExpandedFromMacro()` to check if a call expression originates from a log macro
- For macro-based logging, we may need to lex the original source to extract the log string

Matcher:
```cpp
callExpr(
    anyOf(
        callee(functionDecl(matchesName("*LOG*"))),
        isExpandedFromMacro(matchesName("*LOG*"))
    )
).bind("logCall")
```

For string extraction:
- If arguments are string literals in the AST: extract directly
- If the macro hides the string: use Lexer to read the source text at the macro call site
  and extract string literals or the content inside brackets

For each matched log call:

1. **Get enclosing function**: Traverse up the AST to find the containing `FunctionDecl`
   - If inside a lambda, continue to the outer function
   - Extract function name (and class name for member functions)

2. **Extract log tags**: Use regex to find all `\[([A-Za-z0-9_:]+)\]` patterns
   in the string literal's content

3. **Validate tags**: For each extracted tag:
   - Check if tag equals function name → OK
   - If `AllowQualifiedName=true`, check if tag equals `ClassName::FuncName` → OK
   - Otherwise → WARN

4. **FixIt suggestion**: Replace the mismatched tag with the correct function name

## Class Structure

### LogTagMismatchCheck (public ClangTidyCheck)

```cpp
class LogTagMismatchCheck : public ClangTidyCheck {
public:
  LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context);

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  // Store options
  void storeOptions(ClangTidyOptions::OptionMap& Opts) override;

private:
  // Configuration
  std::vector<std::string> LogMacroPatterns;
  bool AllowQualifiedName;

  // Helpers
  const FunctionDecl* getEnclosingFunction(const Stmt* S, ASTContext* Ctx);
  std::vector<std::string> extractTags(StringRef LogText);
  bool isTagValid(StringRef Tag, const FunctionDecl* Func);
};
```

## Test Cases

### Positive Cases (Should Warn)
```cpp
void FuncA() {
    LOG("[FuncB] test");  // WARN: wrong function name
}

void Foo::Bar() {
    LOG("[Baz] test");    // WARN: wrong method name
}
```

### Negative Cases (Should Not Warn)
```cpp
void FuncA() {
    LOG("[FuncA] test");         // OK: correct name
    LOG("hello world");          // OK: no tag
    LOG("[Foo::FuncA] test");    // OK: qualified name
}

void Foo::FuncA() {
    auto lam = []() {
        LOG("[FuncA] test");     // OK: lambda uses outer function
    };
}
```

## Integration Points

1. Add to `CodelintModule.cpp` with name `"codelint-log-tag-mismatch"`
2. Header file: `include/codelint/checks/LogTagMismatchCheck.h`
3. Source file: `src/codelint_plugin/checks/LogTagMismatchCheck.cpp`
4. Tests: `tests/lit_style_tests_v2/log_tag_mismatch_checker/`

## Dependencies

- Clang AST Matchers
- Clang Lexer (for source text extraction)
- Regex support
