# Codelint Agent Guide

Codelint is a C++ static analysis tool using LLVM LibTooling to detect code issues and suggest fixes.

## Project Overview

- **Language**: C++ (C++26/C++23/C++20)
- **Build System**: CMake
- **Dependencies**: LLVM/Clang 18.1, CLI11, RapidJSON
- **Namespace**: `codelint::lint`
- **Checkers**: init, const, global, singleton

---

## Build Commands

### Initial Build
```bash
mkdir build && cd build && cmake .. && make codelint
```

### Incremental Build
```bash
cd build && make codelint
```

### Clean Rebuild
```bash
cd build && make clean && make codelint
```

---

## Test Commands

### Full Regression (12 tests)
```bash
bash tests/run_regression.sh
```

**Requirement**: `compile_commands.json` at `tests/CodeLintTest/build/`. If missing:
```bash
cd tests/CodeLintTest/build && cmake .. && make
```

### Single Checker Tests
```bash
./codelint lint <file> --only=init
./codelint find_global <file_or_dir>
./codelint find_singleton <file_or_dir>
./codelint check_const <file> [--fix]
./codelint --output-json lint <file>
```

---

## Code Style

### Header Guards
Use `#pragma once` — never `#ifndef`.

### Namespace
All code in `codelint::lint`:
```cpp
namespace codelint { namespace lint {
class MyClass { ... };
} }
```

### Naming
| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `LintChecker` |
| Methods | camelCase | `runOnAST()` |
| Members | trailing underscore | `Context_`, `checker_` |
| Enums | PascalCase values | `Severity::WARNING` |

### Import Order
1. Project headers (`"lint/checker.h"`)
2. Clang/LLVM headers (`"clang/AST/Decl.h"`)
3. Standard library (`<vector>`)

### LibTooling Pattern (ASTConsumer → FrontendAction → FrontendActionFactory)
```cpp
class MyASTConsumer : public clang::ASTConsumer {
public:
    explicit MyASTConsumer(MyChecker *c) : checker_(c) {}
    void HandleTranslationUnit(clang::ASTContext &Ctx) override {
        checker_->runOnAST(&Ctx);
    }
private:
    MyChecker *checker_;
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &CI, llvm::StringRef InFile) override {
        return std::make_unique<MyASTConsumer>(checker_);
    }
private:
    MyChecker *checker_;  // set by factory
};

class MyFactory : public clang::tooling::FrontendActionFactory {
    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<MyFrontendAction>(checker_);
    }
private:
    MyChecker *checker_;
};
```

---

## Directory Structure

```
src/              # Implementation (main.cpp, lint.cpp, checkers/*.cpp)
include/lint/     # Public headers (lint_types.h, lint_checker.h)
tests/            # Test suite + compile_commands.json
third_party/      # CLI11, rapidjson
CMakeLists.txt    # Build config
```

---

## Common Tasks

### Adding a New Checker
1. Create header in `include/lint/checkers/` with `#pragma once`
2. Implement in `src/lint/checkers/mychecker.cpp` using LibTooling pattern
3. Register in checker factory (`init_checker.cpp`)
4. Add source to `CMakeLists.txt`

### Testing
```bash
cd build
./codelint lint ../tests/test.cpp --only=init --fix --output-json
```

---

## Key Constraints

### Test Requirements
- **All 12 regression tests must pass** before marking work complete
- Run: `bash tests/run_regression.sh`

### Linkage Order (Critical - Never Change)
CMakeLists.txt lines 92-98:
```cmake
target_link_libraries(codelint PRIVATE
    ${LLVM_LIBRARY_DIR}/libclang-cpp.so.18.1  # FIRST
    ${LLVM_LIBRARY_DIR}/libclang.so
    ${LLVM_LIBRARY_DIR}/libLLVM.so
)
```

### JSON Format
```json
{
  "issues": [
    {
      "type": "init_uninitialized",
      "severity": "warning",
      "checker": "init",
      "file": "path/to/file.cpp",
      "line": 10,
      "column": 5,
      "description": "...",
      "suggestion": "..."
    }
  ],
  "summary": { "errors": 0, "warnings": 5 }
}
```

### Other Constraints
- All checkers enabled by default in `LintConfig` (`lint_types.h` line 77)
- Severity order: ERROR > WARNING > INFO > HINT
- Use `Severity::WARNING` for most suggestions
- All checkers must implement `name()`, `description()`, `provides()`

---

**Last Updated**: March 2026
