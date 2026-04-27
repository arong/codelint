---
name: clang-tidy
description: >
  Static C++ code analysis with bundled clang-tidy 21.x + codelint plugin.
  When to use: Use this skill when you need fast, deterministic C++ static analysis without API calls.
  Ideal for: CI pipelines, pre-commit hooks, initialization/safety checks, style enforcement.
  Not for: Semantic code review (use glm-review for that), non-C++ languages.
---

# Clang-Tidy Skill

Bundled clang-tidy static analysis with custom codelint plugin for C++ initialization best practices and safety checks.

---

## When to Use

| Scenario | Use This Skill |
|----------|----------------|
| Fast deterministic analysis (no API calls) | Yes |
| CI pipeline or pre-commit hook | Yes |
| Auto-fix initialization/style issues | Yes |
| Detect uninitialized variables | Yes |
| Enforce brace initialization | Yes |
| Semantic code review (design, logic) | No (use glm-review) |
| Non-C++ languages | No |

---

## Quick Start

```bash
# 1. Generate compile_commands.json (required)
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Apply preset config (optional)
cp skills/clang-tidy/configs/.clang-tidy.codelint .clang-tidy

# 3. Set library path
# macOS
export DYLD_LIBRARY_PATH=$PWD/lib:$DYLD_LIBRARY_PATH
# Linux
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

# 4. Run analysis
./skills/clang-tidy/scripts/run_clang_tidy.py -p build -j $(nproc)
```

---

## Core Commands

### Basic Analysis

```bash
# Full project with codelint plugin
./skills/clang-tidy/scripts/run_clang_tidy.py -p build --preset codelint

# Auto-fix issues
./skills/clang-tidy/scripts/run_clang_tidy.py -p build --preset codelint --fix

# Specific checks only
clang-tidy --load=build/lib/codelint-plugin.so \
  --checks='codelint-init' -p build src/*.cpp
```

### PR / Git Diff Analysis

```bash
# Staged changes only
./skills/clang-tidy/scripts/run_clang_tidy_diff.py --staged -p build

# Changes vs main branch
./skills/clang-tidy/scripts/run_clang_tidy_diff.py --branch main -p build
```

### Check Selection

| Goal | Command |
|------|---------|
| Uninitialized variables | `--checks='codelint-init'` |
| Style fixes | `--checks='codelint-lint-code'` |
| Bool conditions | `--checks='codelint-strict-bool-condition'` |
| POSIX signed returns | `--checks='codelint-signed-to-unsigned-return'` |
| All codelint checks | `--checks='codelint-*'` |

---

## See Also

- **Full Reference**: [reference.md](reference.md) - Presets, all checks, AI integration, troubleshooting
- **README**: [README.md](README.md) - Download and setup instructions
