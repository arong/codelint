

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

### 🎯 AI Can ONLY Work on `develop` Branch

**This is a STRICT requirement enforced by git hooks.**

### Branch Permissions

| Branch | AI Status | Reason |
|--------|-----------|--------|
| `develop` | ✅ **ALLOWED** | Designated AI development branch |
| `main` | ❌ **BLOCKED** | Production - humans only |
| `master` | ❌ **BLOCKED** | Production - humans only |
| `production` | ❌ **BLOCKED** | Production - humans only |
| `feature/*` | ⚠️ **WARNED** | Should use develop instead |

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

1. **Always checkout develop first:**
   ```bash
   git checkout develop
   git pull origin develop
   ```

2. **Make all commits on develop:**
   ```bash
   git commit -m "feat: add new functionality"
   ```

3. **Push to develop:**
   ```bash
   git push origin develop
   ```

4. **Let humans handle PRs:**
   - Humans review develop branch
   - Humans create PR: develop → main
   - Humans merge after approval

### Why This Rule?

1. **Isolation** - AI work separate from production
2. **Review** - All AI code gets human review
3. **Stability** - main branch always stable
4. **Traceability** - Clear development path

### Exceptions

**None for AI.** This rule is absolute.

For humans with emergency needs:
```bash
git commit --no-verify -m "emergency fix"
```

---
