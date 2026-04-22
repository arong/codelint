---
name: clang-tidy
description: Static C++ code analysis with bundled clang-tidy + codelint plugin for Linux x86_64.
---

# Clang-Tidy Skill (Linux)

## Quick Start (3 Steps)

```bash
# 1. Generate compile_commands.json (required)
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Copy preset config
cp share/clang-tidy-skill/configs/.clang-tidy.default .clang-tidy

# 3. Run analysis
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
clang-tidy -p build src/**/*.cpp
```

---

## Basic Usage

### PR Review (Changed Files Only)

```bash
./share/clang-tidy-skill/scripts/run-clang-tidy-diff.py --branch main --output sarif
```

### Full Project Scan

```bash
./share/clang-tidy-skill/scripts/run-clang-tidy.py -p build -j $(nproc)
```

### Single File with Auto-fix

```bash
clang-tidy --fix -p build src/main.cpp
```

---

## Configuration Presets

| Preset | Checks | Use Case |
|--------|--------|----------|
| `.clang-tidy.default` | bugprone, modernize, performance | General development |
| `.clang-tidy.strict` | + cert, cppcoreguidelines | Production code |
| `.clang-tidy.security` | cert, security-analyzer | Security audit |

```bash
cp share/clang-tidy-skill/configs/.clang-tidy.strict .clang-tidy
```

---

## Checks Reference

### codelint Plugin Checks

| Check | Auto-fix | Description |
|-------|----------|-------------|
| `codelint-init` | ✅ | Variable initialization style |
| `codelint-strict-bool-condition` | ❌ | Bool-only conditions |
| `codelint-global` | ❌ | Global variable detection |
| `codelint-singleton` | ❌ | Meyer's Singleton pattern |
| `codelint-signed-to-unsigned-return` | ❌ | Signed→unsigned return |

#### codelint-init Details

| Pattern | Before | After |
|---------|--------|-------|
| Uninitialized | `int x;` | `int x{};` |
| Equals syntax | `int x = 5;` | `int x{5};` |
| Missing U suffix | `unsigned u = 1;` | `unsigned u{1U};` |
| Missing UL suffix | `uint64_t n = 42;` | `uint64_t n{42UL};` |

#### codelint-strict-bool-condition

```cpp
// ❌ Error
if (status) {}              // integer in condition
if (strcmp(a, b)) {}        // BUG: returns 0 for equal!

// ✅ OK
if (status == 0) {}
if (strcmp(a, b) == 0) {}
```

#### codelint-signed-to-unsigned-return

```cpp
// ❌ Dangerous: -1 becomes huge positive
size_t n = read(fd, buf, count);

// ✅ Correct
ssize_t n = read(fd, buf, count);
if (n < 0) { handle_error(); }
```

### Built-in clang-tidy

| Category | Key Checks | Auto-fix |
|----------|------------|----------|
| `bugprone-*` | argument-comment | Partial |
| `modernize-*` | loop-convert, use-nullptr | ✅ |
| `performance-*` | for-range-copy | Partial |
| `cert-*` | err34-c | Partial |

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `compile_commands.json not found` | `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` |
| `clang-tidy not found` | Ensure `bin/` in PATH |
| `LD_LIBRARY_PATH error` | `export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH` |
