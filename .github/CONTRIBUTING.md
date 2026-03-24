# Contributing to CodeLint

## Pre-commit Hook

This project uses a pre-commit hook to ensure code quality. The hook automatically runs tests before each commit.

### What the hook checks:

1. **Unit Tests (ctest)** - Runs all GoogleTest unit tests
   - `tests/commands_test.cpp` - Command utilities tests
   - `tests/edge_cases_test.cpp` - Edge case tests
   - `tests/integration_test.cpp` - Integration tests

2. **Regression Tests** - Basic functionality checks
   - `check_init` command finds issues
   - `find_global` command detects globals
   - `find_singleton` command detects singletons
   - JSON output format validation

### Excluded Tests:

The pre-commit hook **does NOT run** tests in:
- `tests/CodeLintTest/src/init_checker/`

These are integration test fixtures used for comprehensive regression testing. They should only be modified when explicitly updating test baselines.

### Installation:

The pre-commit hook is automatically installed when you clone the repository. To manually install or update:

```bash
cd /Users/aronic/Documents/codelint
chmod +x .git/hooks/pre-commit
```

### Bypassing the hook (emergency only):

If you absolutely need to commit without running tests (e.g., fixing a broken test):

```bash
git commit --no-verify -m "your message"
```

**Warning**: Only use `--no-verify` in emergencies. Always fix failing tests before pushing.

### Test Commands:

Run tests manually:

```bash
# Unit tests
ctest --test-dir build --output-on-failure

# Regression tests (full suite)
bash tests/run_regression.sh

# Quick sanity checks
./build/codelint check_init src/main.cpp
./build/codelint find_global tests/
./build/codelint find_singleton tests/
```
