# Codelint for VS Code

Runs [codelint-core](https://github.com/arong/codelint) clang-tidy checks inside VS Code: Problems panel diagnostics, lint on save, and whole-file fixes.

The extension does **not** bundle LLVM or the plugin. It spawns your local `clang-tidy` with `--load=<codelint-core>`.

## Prerequisites

1. Build codelint-core in the repo (or install the shared library somewhere discoverable):

```bash
# macOS
SDKROOT=$(xcrun --show-sdk-path) cmake --preset default
cmake --build build
# → build/lib/codelint-core.dylib
```

2. Install a matching `clang-tidy` (macOS example: `brew install llvm@21`).

3. Generate `compile_commands.json` for the project you are editing (`CMAKE_EXPORT_COMPILE_COMMANDS=ON`, usually under `build/`).

## Develop (F5)

From the **codelint repository root** (recommended):

```bash
cd extensions/vscode-codelint
npm install
npm run compile
```

Open the repository in VS Code/Cursor, then launch **Run Extension** from `.vscode/launch.json` under this folder (or open `extensions/vscode-codelint` as the folder and use the “extension folder” launch config).

## Install on your machine (VSIX)

```bash
cd extensions/vscode-codelint
npm install
npm run compile
npm run package
# or one-shot: build + install into Visual Studio Code
npm run install:vscode
```

In VS Code: **Extensions → … → Install from VSIX…** and select the generated `codelint-*.vsix`.

On this machine, prefer:

```bash
npm run install:vscode
```

(`/usr/local/bin/code` may point at Cursor; the install script targets Visual Studio Code.app.)

`npm run package` uses a small local zip packer (`scripts/pack-vsix.sh`). You can also use `npx @vscode/vsce package` if preferred.

## Settings

| Setting | Default | Meaning |
|---------|---------|---------|
| `codelint.clangTidyPath` | auto | Path to `clang-tidy` |
| `codelint.pluginPath` | auto | Path to `codelint-core.dylib` / `.so` |
| `codelint.compileCommandsDir` | auto | Directory containing `compile_commands.json` |
| `codelint.checks` | `codelint-*` | `--checks` pattern |
| `codelint.runOnSave` | `true` | Lint active C/C++ file on save |
| `codelint.fixOnSave` | `false` | Apply `--fix` on save |

Example (absolute paths on this machine):

```json
{
  "codelint.clangTidyPath": "/opt/homebrew/opt/llvm@21/bin/clang-tidy",
  "codelint.pluginPath": "/Users/aronic/Documents/codelint/build/lib/codelint-core.dylib",
  "codelint.compileCommandsDir": "/Users/aronic/Documents/codelint/build",
  "codelint.runOnSave": true
}
```

Auto-discovery looks under the workspace for `build/lib/codelint-core.{dylib,so}` and `build/compile_commands.json`.

## Commands

- **Codelint: Lint Active File**
- **Codelint: Fix Active File** (`clang-tidy --fix`)
- **Codelint: Lint Workspace** (sequential)
- **Codelint: Show Output**

Quick Fix on a Codelint diagnostic also offers **Fix file with Codelint**.

## Troubleshooting

Open **Codelint: Show Output** to see the exact command, stdout, and stderr.

- Plugin not found → build the repo or set `codelint.pluginPath`.
- Wrong includes / false positives → set `codelint.compileCommandsDir` to the folder that contains `compile_commands.json`.
- `clang-tidy` version should match the LLVM used to build `codelint-core`.
