#!/bin/bash
# One-time setup after cloning the repository (local or Cursor Cloud)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR"
HOOKS_DIR="$REPO_ROOT/.githooks"

echo "Setting up development environment..."
echo "  repo:  $REPO_ROOT"
echo "  hooks: $HOOKS_DIR"

if [[ ! -d "$HOOKS_DIR" ]]; then
  echo "ERROR: hooks directory not found: $HOOKS_DIR"
  exit 1
fi

for hook in pre-commit commit-msg; do
  if [[ ! -f "$HOOKS_DIR/$hook" ]]; then
    echo "ERROR: missing hook: $HOOKS_DIR/$hook"
    exit 1
  fi
done

chmod +x "$HOOKS_DIR/pre-commit" "$HOOKS_DIR/commit-msg"

# Absolute path so hooks resolve reliably across cwd / cloud agents.
# Prefer per-worktree config so sibling worktrees do not overwrite each other.
git -C "$REPO_ROOT" config extensions.worktreeConfig true
if git -C "$REPO_ROOT" config --worktree core.hooksPath "$HOOKS_DIR" 2>/dev/null; then
  echo "Configured worktree core.hooksPath=$(git -C "$REPO_ROOT" config --get core.hooksPath)"
else
  git -C "$REPO_ROOT" config core.hooksPath "$HOOKS_DIR"
  echo "Configured local core.hooksPath=$(git -C "$REPO_ROOT" config --get core.hooksPath)"
fi

echo ""
echo "Active hooks:"
echo "  - pre-commit: clang-format, whitespace checks, sensitive files"
echo "  - commit-msg: format validation, branch restriction (blocked: main/master/production/release)"
echo ""
echo "Setup complete."
