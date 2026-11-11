#!/usr/bin/env bash
# Build VSIX and install into the real Visual Studio Code app (not Cursor's `code` shim).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

npm run compile
npm run package

VSIX="$(ls -1 "$ROOT"/codelint-*.vsix | sort -V | tail -1)"
VSCODE_BIN="/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"

if [[ ! -x "$VSCODE_BIN" ]]; then
  echo "ERROR: Visual Studio Code CLI not found at: $VSCODE_BIN" >&2
  exit 1
fi

echo "Installing $VSIX into Visual Studio Code..."
"$VSCODE_BIN" --install-extension "$VSIX" --force
"$VSCODE_BIN" --list-extensions | grep -i codelint || true
echo "Done. Reload the VS Code window to pick up the new build."
