# Agent notes

Detailed always-on rules live in [`.cursor/rules/`](.cursor/rules/).

## Cursor Cloud specific instructions

Cloud agents use [`.cursor/Dockerfile`](.cursor/Dockerfile) (Ubuntu 22.04 + **LLVM 15**) via [`.cursor/environment.json`](.cursor/environment.json).

```bash
./setup.sh
cmake --preset linux-llvm15
cmake --build build -j$(nproc)
bash tests/run_lit_tests.sh
```

- Branch in-place: `git switch -c feature/<name>` (no sibling worktree required).
- Do not use `cmake --preset default` here — it expects LLVM 21 on Linux.
