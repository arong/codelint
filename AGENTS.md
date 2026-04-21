---
## 🤖 AI Commit Author (CRITICAL)

**AI must use distinct author for commits.**

```
Sisyphus <sisyphus@codelint.dev>
```

```bash
git commit --author="Sisyphus <sisyphus@codelint.dev>" -m "message"
```

---

## ⚠️ Do NOT Bypass Git Hooks

**Git hooks enforce critical checks. NEVER use `--no-verify` unless explicitly requested.**

Pre-commit checks:
- ✅ clang-format (auto-fixes C++ files)
- ✅ trailing whitespace/newlines
- ✅ sensitive files detection

Commit-msg checks:
- ✅ commit message format (`type: description`)
- ✅ branch restriction (AI only on `develop`)
- ✅ quick tests on develop

---

## 🚫 Test File Protection (CRITICAL)

**NEVER modify these directories:**

```
tests/CodeLintTest/src/init_checker/src/*.cpp    - Source fixtures
tests/CodeLintTest/src/init_checker/fixed/*.cpp  - Expected output
```

When tests fail: Fix the CODE, not the tests.

---

## 🎯 AI Branch Rules

**AI can ONLY commit on `develop` branch.**

Blocked: `main`, `master`, `production`

Workflow:
1. `git checkout develop && git pull origin develop`
2. Make commits on develop
3. `git push origin develop`
4. Humans create PR: develop → main

---

## 🔄 PR Submission Protocol

**When asked to "submit PR":**

1. Create PR: `gh pr create --base main --head develop`
2. **MUST watch CI**: `gh run watch --exit-status`
3. Fix failures before reporting done

**NEVER:**
- ❌ Report "done" before CI passes
- ❌ Leave CI failures for user to fix

---

## 🔧 Build & LLVM Paths

```bash
# macOS
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default
cmake --build build

# Linux  
cmake --preset default
cmake --build build
```

LLVM 21 path:
- macOS: `/opt/homebrew/opt/llvm@21/bin/`
- Linux: `/usr/lib/llvm-21/bin/`

---

## 🔍 CI Debug Commands

```bash
gh run list --limit 1                    # Latest CI status
gh run watch <ID> --exit-status          # Watch until done
gh run view <ID> --log-failed            # View failures
```

---