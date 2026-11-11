#!/usr/bin/env bash
# Build VSIX and install into the real Visual Studio Code app (not Cursor's `code` shim).
#
# Local iteration does NOT require bumping package.json version.
# Same version + --force overwrites the installed build.
# Only bump version when you intentionally cut a release.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(node -p "require('./package.json').version")"
NAME="$(node -p "require('./package.json').name")"
echo "Building ${NAME}@${VERSION} (version unchanged unless you bumped package.json)"

# Drop stale VSIX artifacts so the folder doesn't fill with old builds.
rm -f "$ROOT"/*.vsix

npm run compile
npm run package

VSIX="$ROOT/${NAME}-${VERSION}.vsix"
if [[ ! -f "$VSIX" ]]; then
  echo "ERROR: expected VSIX not found: $VSIX" >&2
  exit 1
fi

VSCODE_BIN="/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"
if [[ ! -x "$VSCODE_BIN" ]]; then
  echo "ERROR: Visual Studio Code CLI not found at: $VSCODE_BIN" >&2
  exit 1
fi

echo "Installing $VSIX into Visual Studio Code (force overwrite)..."
"$VSCODE_BIN" --install-extension "$VSIX" --force
"$VSCODE_BIN" --list-extensions --show-versions | grep -i codelint || true
echo "Done. Reload the VS Code window (or Restart Extensions) to pick up the build."
echo "Tip: keep package.json version stable while iterating; bump only for releases."
