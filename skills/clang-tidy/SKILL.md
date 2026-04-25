---
name: clang-tidy
description: Static C++ code analysis with bundled clang-tidy 21.x + codelint plugin. Supports macOS (arm64/x64) and Linux (x64).
---

# Clang-Tidy Skill

Bundled clang-tidy static analysis solution with custom codelint plugin for initialization best practices and code safety checks.

## Supported Platforms

| Platform | Architecture | Package |
|----------|-------------|---------|
| macOS | Apple Silicon (arm64) | `clang-tidy-skill-*-darwin-arm64.tar.gz` |
| macOS | Intel (x64) | `clang-tidy-skill-*-darwin-x64.tar.gz` |
| Linux | x86_64 | `clang-tidy-skill-*-linux-x64.tar.gz` |

---

## Quick Start

```bash
# 1. Generate compile_commands.json (required for accurate analysis)
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Copy preset config (optional but recommended)
cp skills/clang-tidy/configs/.clang-tidy.codelint .clang-tidy

# 3. Run analysis
# macOS
export DYLD_LIBRARY_PATH=$PWD/lib:$DYLD_LIBRARY_PATH
# Linux
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

./skills/clang-tidy/scripts/run_clang_tidy.py -p build -j $(nproc)
```

---

## Configuration Presets

| Preset | Checks | Use Case |
|--------|--------|----------|
| `.clang-tidy.codelint` | **codelint-* only** | Focused initialization/safety audit |
| `.clang-tidy.default` | bugprone, modernize, performance | General development |
| `.clang-tidy.strict` | + cert, cppcoreguidelines | Production code |
| `.clang-tidy.security` | cert, clang-analyzer-security | Security audit |

```bash
# Apply preset
cp skills/clang-tidy/configs/.clang-tidy.codelint .clang-tidy
```

---

## codelint Plugin Checks (7 total)

| Check | Auto-fix | Language | Description |
|-------|----------|----------|-------------|
| `codelint-init` | ✅ Partial | C++14/17/20 | Uninitialized variables, dangerous conversions |
| `codelint-lint-code` | ✅ Full | C++14/17/20 | Style: `=` → `{}`, unsigned suffix |
| `codelint-strict-bool-condition` | ❌ No | C++14-20 ⚠️ | Bool-only conditions (excludes C++23) |
| `codelint-signed-to-unsigned-return` | ❌ No | C++14/17/20 | POSIX signed return → unsigned |
| `codelint-global` | ❌ No | C++ | Global variable detection |
| `codelint-global-const-string` | ❌ No | C++ | Global const string → constexpr |
| `codelint-singleton` | ❌ No | C++ | Meyer's Singleton pattern |

### codelint-init

**Focus**: Correctness and safety (Error-level diagnostics)

| Pattern | Before | After |
|---------|--------|-------|
| Uninitialized trivial | `int x;` | `int x{};` ⚠️ Error |
| Uninitialized non-trivial | `std::string s;` | `std::string s{};` ⚠️ Warning |
| int→bool conversion | `bool b = 1;` | Manual fix ⚠️ Error |
| Narrowing | `int x = 3.14;` | Manual fix ⚠️ Warning |
| C-style array | `int arr[5];` | `int arr[5]{};` |

**Smart skips**: for-loops, catch params, macros, extern, unions, references, bitfields

### codelint-lint-code

**Focus**: Code style (Warning-level diagnostics)

| Pattern | Before | After |
|---------|--------|-------|
| Non-auto `=` syntax | `int x = 5;` | `int x{5};` |
| Auto `{}` syntax | `auto x{42};` | `auto x = 42;` |
| Missing U suffix | `unsigned u = 1;` | `unsigned u{1U};` |
| Missing UL suffix | `uint64_t n = 42;` | `uint64_t n{42UL};` |
| `= {}` syntax | `int x = {};` | `int x{};` |

**Why auto uses `=`**: Brace init with auto can deduce to `std::initializer_list`, causing surprising type deductions.

### codelint-strict-bool-condition

```cpp
// ❌ Error - dangerous implicit conversions
if (status) {}              // integer in condition
if (strcmp(a, b)) {}        // BUG: returns 0 for equal strings!
while (ptr) {}              // pointer in condition
bool r = val ? a : b;       // non-bool in ternary

// ✅ OK - explicit comparisons
if (status == 0) {}
if (strcmp(a, b) == 0) {}
while (ptr != nullptr) {}
```

⚠️ **Not supported**: C++23 (may have incompatible features)

### codelint-signed-to-unsigned-return

```cpp
// ❌ Dangerous: -1 becomes SIZE_MAX
size_t n = read(fd, buf, count);
unsigned fd = open("/path", O_RDONLY);

// ✅ Correct: Use signed type, check for error
ssize_t n = read(fd, buf, count);
if (n < 0) { handle_error(); }
```

**Checked POSIX functions**: `read`, `write`, `open`, `close`, `stat`, `mmap`, `fork`, `socket`, `recv`, `send`, etc.

### codelint-global / codelint-singleton

```cpp
// codelint-global: Detected
int global_counter = 0;
namespace { int config = 42; }

// codelint-singleton: Detected Meyer's pattern
class Singleton {
  static Singleton& instance() {
    static Singleton inst;  // ← flagged
    return inst;
  }
};
```

### codelint-global-const-string

```cpp
// Detected - suggests constexpr
const std::string kAppName = "MyApp";
// → constexpr const char* kAppName = "MyApp";
```

---

## Built-in clang-tidy (via presets)

| Category | Key Checks | Auto-fix |
|----------|------------|----------|
| `bugprone-*` | argument-comment, dangling-handle | Partial |
| `modernize-*` | loop-convert, use-nullptr | ✅ Full |
| `performance-*` | for-range-copy, move-const-arg | Partial |
| `cert-*` | err34-c, fio38-c | Partial |
| `cppcoreguidelines-*` | pro-type-member-init | Partial |

---

## Git Diff Analysis

Analyze only changed files for PR review:

```bash
# Staged changes (pre-commit)
./skills/clang-tidy/scripts/run_clang_tidy_diff.py --staged -p build

# Changes vs main branch
./skills/clang-tidy/scripts/run_clang_tidy_diff.py --branch main --output sarif > results.sarif

# Recent commits
./skills/clang-tidy/scripts/run_clang_tidy_diff.py --commits 5 -p build
```

---

## AI Integration Guide

### Quick Call Examples

```bash
# Full project scan with codelint plugin
clang-tidy --load=build/lib/codelint-plugin.so --checks='codelint-*' -p build src/**/*.cpp

# Auto-fix style issues (codelint-init + codelint-lint-code)
clang-tidy --load=build/lib/codelint-plugin.so --checks='codelinit-*' --fix -p build src/**/*.cpp

# Security-focused: only dangerous conversions
clang-tidy --load=build/lib/codelint-plugin.so \
  --checks='codelinit-init,codelint-strict-bool-condition,codelint-signed-to-unsigned-return' \
  -p build src/**/*.cpp

# PR review: changed files only
git diff --name-only --diff-filter=ACM main | \
  grep -E '\.(cpp|cc|h|hpp)$' | \
  xargs clang-tidy --load=build/lib/codelint-plugin.so -p build

# Using wrapper script (auto-finds plugin)
./skills/clang-tidy/scripts/run_clang_tidy.py -p build --preset codelint --fix
```

### Check Selection Strategy

| Scenario | Recommended Checks | Reason |
|----------|-------------------|--------|
| Initial cleanup | `codelint-init` + `--fix` | Fixes uninitialized, auto-fixable |
| Style enforcement | `codelint-lint-code` + `--fix` | Brace init, unsigned suffix |
| Critical safety | `codelint-init,codelint-strict-bool-condition,codelint-signed-to-unsigned-return` | Error-level checks only |
| Architecture review | `codelint-global,codelint-singleton` | Global state detection |
| Full audit | `codelint-*` | All checks enabled |

### Fix Verification Flow

```
1. Run analysis without --fix → capture warnings
2. Apply --fix → auto-fix style issues
3. Re-run analysis → verify no remaining warnings
4. Manual review Error-level diagnostics (int→bool, narrowing)
5. Build → ensure no compilation errors
6. Tests → ensure behavior unchanged
```

```bash
# Step 1: Initial scan
clang-tidy --load=lib/codelint-plugin.so -p build src/*.cpp > issues.txt

# Step 2: Apply fixes
clang-tidy --load=lib/codelint-plugin.so --fix -p build src/*.cpp

# Step 3: Verify fixes
clang-tidy --load=lib/codelint-plugin.so -p build src/*.cpp

# Step 4-5: Build and test
cmake --build build && ctest --test-dir build
```

### Common Issues & Solutions

| Issue | Diagnosis | Solution |
|-------|-----------|----------|
| `compile_commands.json not found` | CMake didn't export | `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| `Analysis fails with symbol errors` | Library path not set | macOS: `export DYLD_LIBRARY_PATH=...` / Linux: `export LD_LIBRARY_PATH=...` |
| `Plugin not loaded` | Wrong plugin path | Check `build/lib/codelint-plugin.so` or `build/lib/codelint-plugin.dylib` |
| `--fix causes compile errors` | Fix broke syntax | Review changes, manually adjust edge cases |
| `Too many warnings` | Check set too broad | Narrow to specific checks: `--checks='codelint-init'` |
| `Unchanged files reported` | Header filter too broad | Set `HeaderFilterRegex: '(src/|include/).*'` in .clang-tidy |
| `C++23 code fails` | strict-bool-condition not supported | Disable: `--checks='codelint-*,-codelint-strict-bool-condition'` |

---

## CI Integration

### GitHub Actions

```yaml
- name: Run clang-tidy
  run: |
    ./skills/clang-tidy/scripts/run_clang_tidy_diff.py \
      --branch ${{ github.base_ref }} \
      --output sarif \
      > results.sarif

- name: Upload SARIF
  uses: github/codeql-action/upload-sarif@v3
  with:
    sarif_file: results.sarif
```

### GitLab CI

```yaml
lint:
  script:
    - ./skills/clang-tidy/scripts/run_clang_tidy.py -p build --preset strict
  artifacts:
    reports:
      codequality: clang-tidy-report.json
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `clang-tidy not found` | Install: `brew install llvm@21` (macOS) or `apt install clang-tidy-21` (Linux) |
| `Plugin load fails` | Check library path and plugin architecture matches platform |
| `Wrong check results` | Verify `.clang-tidy` config is in project root |
| `Fix doesn't apply` | Only `codelint-init` (uninit) and `codelint-lint-code` support auto-fix |
| `Warning level differs` | Trivial types = Error, non-trivial = Warning for uninitialized |
