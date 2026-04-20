

---

## 🤖 AI Commit Author (CRITICAL)

**AI must use distinct author for commits.**

### Author

```
Sisyphus <sisyphus@codelint.dev>
```

### Usage

```bash
# Using alias
git ai-commit -m "style: fix clang-tidy warnings"

# Or manually
git commit --author="Sisyphus <sisyphus@codelint.dev>" -m "message"
```

### Verify

```bash
git log -1 --format="%an <%ae>"
# Output: Sisyphus <sisyphus@codelint.dev>
```

---

## Test File Protection Rules (CRITICAL)

### Protected Test Directories

The following directories contain **TEST FIXTURES** that MUST NOT be modified:

```
🚫 PROTECTED - DO NOT MODIFY:
- tests/CodeLintTest/src/init_checker/src/*.cpp    - Source test fixtures
- tests/CodeLintTest/src/init_checker/fixed/*.cpp  - Expected output fixtures
- tests/CodeLintTest/build/                         - Build artifacts
- tests/compile_commands.json                       - Test build config
```

### What AI MUST NOT Do

❌ **NEVER modify files in protected directories**
- Do NOT change `.cpp` source files in `tests/CodeLintTest/src/init_checker/`
- Do NOT change `.cpp` files in `tests/CodeLintTest/src/init_checker/fixed/`
- Do NOT modify test baseline expectations
- Do NOT "fix" test files to make tests pass

❌ **NEVER change test behavior to pass**
- Do NOT skip tests that are failing
- Do NOT modify test comparison logic to ignore differences
- Do NOT change expected output formats

### What AI CAN Do

✅ **Safe modifications**:
- Add NEW test files in `tests/CodeLintTest/src/init_checker/src/` and `tests/CodeLintTest/src/init_checker/fixed/` (must add both src and fixed files with matching names)
- Add NEW test files in `tests/` (e.g., `tests/commands_test.cpp`, `tests/integration_test.cpp`)
- Modify test runner scripts (`tests/run_regression.sh`) for better reporting
- Modify test infrastructure (CMakeLists.txt for test configuration)
- Add new test categories or test suites

✅ **When tests fail**:
1. Check if the failure is due to code changes (fix the CODE, not the test)
2. Check if it's a pre-existing test issue (document it, don't hide it)
3. Report the failure to the user for decision

### Rationale

Test fixtures in `tests/CodeLintTest/src/init_checker/` are **golden files** - they define the expected behavior of the tool. Modifying them:

1. ❌ Breaks regression testing
2. ❌ Hides real bugs
3. ❌ Makes it impossible to verify behavior consistency
4. ❌ Violates testing best practices

### Enforcement

The pre-commit hook will verify:
- Unit tests pass (GoogleTest in `tests/`)
- Basic functionality works (check_init, find_global, find_singleton)

The pre-commit hook intentionally **excludes** `tests/CodeLintTest/src/init_checker/` tests
because these should only be updated when explicitly changing tool behavior.

### When You Need to Update Protected Tests

If you genuinely need to update protected test files:

1. **Ask the user first** - Explain WHY the test needs to change
2. **Document the change** - Record the rationale in commit message
3. **Use explicit command** - Only modify when user explicitly requests it

Example commit message when updating protected tests:
```
test: update init_checker baseline for new feature X

- Updated fixed/init_check.cpp to reflect new behavior
- New feature X changes initialization detection rules
- User approved this change in issue #123
```

---

## AI Behavior Checklist

Before making ANY change, AI should verify:

- [ ] Am I modifying test files? → STOP and check if they're protected
- [ ] Am I changing test expectations? → This is FORBIDDEN without explicit approval
- [ ] Are tests failing? → Fix the code, not the tests
- [ ] Do I need to update baselines? → Ask the user first

**REMEMBER**: Tests define correctness. Changing tests to pass = hiding bugs.

---

## AI Development Branch Rules (CRITICAL)

### 🎯 AI Can Work on `develop` or `skills` Branch

**This is a STRICT requirement enforced by git hooks.**

### Branch Permissions

| Branch | AI Status | Reason |
|--------|-----------|--------|
| `develop` | ✅ **ALLOWED** | Designated AI development branch |
| `skills` | ✅ **ALLOWED** | AI skill development branch |
| `main` | ❌ **BLOCKED** | Production - humans only |
| `master` | ❌ **BLOCKED** | Production - humans only |
| `production` | ❌ **BLOCKED** | Production - humans only |
| `feature/*` | ❌ **BLOCKED** | Should use develop/skills instead |

### Enforcement

The `commit-msg` hook enforces this rule:

**When you commit on `develop`:**
```
✓ All tests passed. Commit proceeding.
```

**When you try to commit on `main`:**
```
╔════════════════════════════════════════╗
║  🚫 BLOCKED: AI cannot commit to main  ║
╚════════════════════════════════════════╝
```

### Required Workflow

AI assistants MUST:

1. **Always checkout develop or skills first:**
   ```bash
   git checkout develop
   git pull origin develop
   # Or for skill development
   git checkout skills
   git pull origin skills
   ```

2. **Make all commits on develop/skills:**
   ```bash
   git commit -m "feat: add new functionality"
   ```

3. **Push to develop/skills:**
   ```bash
   git push origin develop
   # Or
   git push origin skills
   ```

4. **Let humans handle PRs:**
   - Humans review develop/skills branch
   - Humans create PR: develop/skills → main
   - Humans merge after approval

### Why This Rule?

1. **Isolation** - AI work separate from production
2. **Review** - All AI code gets human review
3. **Stability** - main branch always stable
4. **Traceability** - Clear development path

### Exceptions

**None for AI on main/master/production branches.** This rule is absolute.

For humans with emergency needs:
```bash
git commit --no-verify -m "emergency fix"
```

---

## 📋 PR Merge Checklist (CRITICAL)

**ALWAYS test the release workflow before merging!**

Release workflow is triggered by tags only. Test it with a test tag:

```bash
git checkout develop
git tag v0.X.X-test
git push origin v0.X.X-test
gh run watch --exit-status
gh release delete v0.X.X-test --cleanup-tag --yes
```

**Checklist before merge:**
- [ ] Code Check workflow passes
- [ ] Test tag release workflow passes
- [ ] Package tested (`./bin/codelint --version`, `--list-checks`)
- [ ] Test release cleaned up

---

## 🔧 CMake Presets (Build Configuration)

**This project uses CMakePresets.json for platform-specific configuration.**

### Available Presets

| Preset | Platform | Description |
|--------|----------|-------------|
| `default` | Auto | Auto-detect platform (macOS/Linux) |
| `macos` | macOS | macOS with Homebrew LLVM 21 |
| `linux` | Linux | Linux with apt.llvm.org LLVM 21 |

### How to Build

**Using preset (recommended):**
```bash
# macOS - set SDKROOT first
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default

# Linux
cmake --preset default

# Build
cmake --build build
```

**Manual configuration (fallback):**
```bash
# macOS
cmake -B build -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)

# Linux
cmake -B build -DLLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm

cmake --build build
```

### Why Use Presets

1. **Platform Detection** - Automatic macOS/Linux configuration
2. **SDK Path** - macOS SDK path for system headers (clang-tidy compatibility)
3. **LLVM Path** - Correct LLVM 21 path for each platform
4. **Reproducibility** - Consistent build configuration across environments

---

## 🔧 LLVM/Clang Tool Paths (CRITICAL)

**This project uses LLVM 21. All LLVM tools must use this specific path.**

### macOS (Homebrew)

```bash
LLVM_BIN=/opt/homebrew/opt/llvm@21/bin
LLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm
```

### Linux (apt.llvm.org)

```bash
LLVM_BIN=/usr/lib/llvm-21/bin
LLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm
```

### Tools in This Directory

| Tool | macOS Path | Linux Path |
|------|------------|------------|
| `clang` | `/opt/homebrew/opt/llvm@21/bin/clang` | `/usr/lib/llvm-21/bin/clang` |
| `clang++` | `/opt/homebrew/opt/llvm@21/bin/clang++` | `/usr/lib/llvm-21/bin/clang++` |
| `clang-tidy` | `/opt/homebrew/opt/llvm@21/bin/clang-tidy` | `/usr/lib/llvm-21/bin/clang-tidy` |
| `llvm-cov` | `/opt/homebrew/opt/llvm@21/bin/llvm-cov` | `/usr/lib/llvm-21/bin/llvm-cov` |
| `llvm-profdata` | `/opt/homebrew/opt/llvm@21/bin/llvm-profdata` | `/usr/lib/llvm-21/bin/llvm-profdata` |
| `clang-format` | `/opt/homebrew/opt/llvm@21/bin/clang-format` | `/usr/lib/llvm-21/bin/clang-format` |

### How to Use

**Before running any LLVM tool, set PATH:**
```bash
# macOS
export PATH=/opt/homebrew/opt/llvm@21/bin:$PATH

# Linux
export PATH=/usr/lib/llvm-21/bin:$PATH
```

**Or use full paths directly:**
```bash
clang-tidy --version  # After setting PATH
```

### Why This Matters

1. **Consistency** - Ensures all tools are from the same LLVM version
2. **Compatibility** - Plugin requires LLVM 21 APIs
3. **Coverage** - llvm-cov must match the compiler version used for instrumentation

---

## 🎨 C++ Code Formatting (MANDATORY)

**After modifying ANY C++ source file, you MUST run clang-format.**

### Required Workflow

```
1. Edit C++ file(s)
2. Run clang-format on modified files
3. Build and test to verify changes
```

### clang-format Command

```bash
# Format specific files
clang-format -i src/codelint_plugin/checks/InitCheck.cpp

# Format all modified C++ files in src/
clang-format -i src/**/*.cpp

# Or using full path (recommended)
/opt/homebrew/opt/llvm@21/bin/clang-format -i <modified_files>
```

### Project Style Configuration

The project uses `.clang-format` with:
- **Base Style**: LLVM
- **Indent Width**: 2 spaces
- **Column Limit**: 100
- **Pointer Alignment**: Left

### Why This Matters

1. **Consistency** - All code follows the same style
2. **Readability** - Proper indentation and spacing
3. **Pre-commit Hook** - Fails if formatting is incorrect
4. **Review Quality** - Cleaner diffs for human review

### Files to Format

| Directory | Format Required |
|-----------|-----------------|
| `src/**/*.cpp` | ✅ Yes |
| `src/**/*.h` | ✅ Yes |
| `include/**/*.h` | ✅ Yes |
| `tests/**/*.cpp` | ✅ Yes (except protected fixtures) |

**⚠️ EXCEPTION**: Do NOT format files in `tests/CodeLintTest/src/init_checker/src/` or `tests/CodeLintTest/src/init_checker/fixed/` - these are protected test fixtures.

---

## 🔧 GitHub CLI (gh) Usage Guide for CI Debugging

**IMPORTANT: gh CLI is available on this machine!** Always use it to check CI status and debug failures.

### 🔍 Check CI Status

```bash
# List recent CI runs (show latest 1)
gh run list --limit 1

# List all runs for current branch
gh run list --branch develop --limit 10
```

### 📖 View CI Run Details

```bash
# View run status and jobs
gh run view <RUN_ID>

# View failed run logs
gh run view <RUN_ID> --log

# View specific failed step logs
gh run view <RUN_ID> --log --failed

# View logs from a specific job
gh run view <RUN_ID> --job <JOB_ID> --log
```

### 🚨 Common Commands for CI Debugging

```bash
# Check if latest CI passed
gh run list --limit 1 | grep completed

# Rerun failed workflow
gh run rerun <FAILED_RUN_ID>

# Watch CI run in real-time
gh run watch <RUN_ID>
```

### 📊 Extract Error Information

```bash
# Get only error lines from logs
gh run view <RUN_ID> --log | grep "ERROR:\|##\[error\]"

# Check if tests passed
gh run view <RUN_ID> --log | grep -E "(Total tests|Passed|Failed)"

# Find specific failure
gh run view <RUN_ID> --log | grep -B3 -A3 "FAIL:"
```

### 🛠️ Typical CI Debugging Workflow

```bash
# 1. Check latest CI status
gh run list --limit 1

# 2. View detailed run info
gh run view <RUN_ID>

# 3. Check which jobs failed
gh run view <RUN_ID> | grep -E "(JOBS|X [a-z-]+)"

# 4. Get failure logs
gh run view <RUN_ID> --log | grep -B5 "##\[error\]"

# 5. Focus on specific failure type
gh run view <RUN_ID> --log | grep -E "(trailing whitespace|trailing newline|clang-tidy|FAIL:)"
```

### 📝 CI Run ID Examples

Run IDs look like: `24257528758`, `24258215019`, etc.

To find the latest run ID:
```bash
gh run list --limit 1 --json databaseId | jq -r '.[0].databaseId'
```

### 🎯 Quick Status Check Commands

```bash
# Is latest CI passing?
gh run list --limit 1 --json conclusion --jq '.[0].conclusion'
# Output: "success" or "failure"

# Get job names that failed
gh run view <RUN_ID> --json jobs --jq '.jobs[] | select(.conclusion=="failure") | .name'
```

---
