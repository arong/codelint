import * as path from "path";
import * as vscode from "vscode";
import { CodelintCodeActionProvider } from "./codeActions";
import { hashFileContent, LintCache, makeConfigKey } from "./cache";
import { resolveConfig } from "./config";
import { DecorationsManager } from "./decorations";
import { DiagnosticsManager } from "./diagnostics";
import { parseClangTidyOutput } from "./parser";
import { CodelintRunner } from "./runner";

const CPP_GLOB = "**/*.{c,cc,cpp,cxx,C,h,hh,hpp,hxx}";

let output: vscode.OutputChannel;
let runner: CodelintRunner;
let decorations: DecorationsManager;
let diagnostics: DiagnosticsManager;
let lintCache: LintCache;
let statusBar: vscode.StatusBarItem;
let saveTimer: NodeJS.Timeout | undefined;
let linting = false;

export function activate(context: vscode.ExtensionContext): void {
  output = vscode.window.createOutputChannel("Codelint");
  runner = new CodelintRunner(output);
  decorations = new DecorationsManager();
  diagnostics = new DiagnosticsManager(decorations);
  lintCache = new LintCache(64);
  statusBar = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
  statusBar.command = "codelint.showOutput";
  statusBar.text = "$(check) Codelint";
  statusBar.tooltip = "Codelint is active — click to show output";
  statusBar.show();
  output.appendLine("Codelint extension activated.");

  context.subscriptions.push(
    output,
    statusBar,
    decorations,
    diagnostics.collectionHandle,
    vscode.commands.registerCommand("codelint.lintActiveFile", () =>
      lintActiveFile(false)
    ),
    vscode.commands.registerCommand("codelint.fixActiveFile", (uri?: vscode.Uri) =>
      fixFile(uri)
    ),
    vscode.commands.registerCommand("codelint.lintWorkspace", () => lintWorkspace()),
    vscode.commands.registerCommand("codelint.showOutput", () => output.show(true)),
    vscode.commands.registerCommand("codelint.clearCache", () => {
      lintCache.clear();
      output.appendLine("Lint cache cleared.");
      vscode.window.showInformationMessage("Codelint: cache cleared.");
    }),
    vscode.languages.registerCodeActionsProvider(
      [
        { language: "cpp", scheme: "file" },
        { language: "c", scheme: "file" },
      ],
      new CodelintCodeActionProvider(),
      { providedCodeActionKinds: CodelintCodeActionProvider.providedCodeActionKinds }
    ),
    vscode.workspace.onDidSaveTextDocument((doc) => scheduleLint(doc, "save")),
    vscode.workspace.onDidOpenTextDocument((doc) => scheduleLint(doc, "open")),
    vscode.window.onDidChangeActiveTextEditor((editor) => {
      if (editor) {
        scheduleLint(editor.document, "open");
        decorations.refreshVisibleEditors();
      }
    }),
    vscode.window.onDidChangeVisibleTextEditors(() => {
      decorations.refreshVisibleEditors();
    }),
    vscode.workspace.onDidChangeConfiguration((e) => {
      if (e.affectsConfiguration("codelint")) {
        lintCache.clear();
        output.appendLine("Codelint settings changed — cache cleared.");
      }
    }),
    {
      dispose: () => {
        if (saveTimer) {
          clearTimeout(saveTimer);
        }
        runner.cancel();
        diagnostics.dispose();
        lintCache.clear();
      },
    }
  );

  // Lint the already-open editor (activation often happens after the file is open).
  if (vscode.window.activeTextEditor) {
    scheduleLint(vscode.window.activeTextEditor.document, "open");
  }
}

export function deactivate(): void {
  runner?.cancel();
}

function isCppLike(doc: vscode.TextDocument): boolean {
  return (
    doc.uri.scheme === "file" &&
    (doc.languageId === "cpp" || doc.languageId === "c")
  );
}

function cacheEnabled(): boolean {
  return vscode.workspace.getConfiguration("codelint").get<boolean>("cache", true);
}

function scheduleLint(doc: vscode.TextDocument, reason: "save" | "open"): void {
  if (!isCppLike(doc)) {
    return;
  }

  let config;
  try {
    config = resolveConfig();
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    output.appendLine(`config error (${reason}): ${message}`);
    void vscode.window.showWarningMessage(`Codelint: ${message}`);
    return;
  }

  if (reason === "save") {
    if (!config.runOnSave && !config.fixOnSave) {
      return;
    }
  } else if (!config.runOnSave) {
    return;
  }

  // Fast path: on open/switch, reuse cached diagnostics without waiting on clang-tidy.
  if (reason === "open" && cacheEnabled() && tryApplyCache(doc.uri.fsPath, config)) {
    return;
  }

  if (saveTimer) {
    clearTimeout(saveTimer);
  }
  const delayMs = reason === "open" ? 300 : 500;
  saveTimer = setTimeout(async () => {
    try {
      const shouldFix = reason === "save" && config.fixOnSave;
      // Save always rechecks (content changed).
      await runOnPath(doc.uri.fsPath, shouldFix, {
        force: reason === "save" || shouldFix,
      });
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      output.appendLine(`${reason} error: ${message}`);
    }
  }, delayMs);
}

function tryApplyCache(
  filePath: string,
  config: ReturnType<typeof resolveConfig>
): boolean {
  try {
    const contentHash = hashFileContent(filePath);
    const configKey = makeConfigKey(config);
    const cached = lintCache.get(filePath, contentHash, configKey);
    if (!cached) {
      return false;
    }
    diagnostics.setForFile(filePath, cached);
    decorations.refreshVisibleEditors();
    statusBar.text =
      cached.length > 0
        ? `$(warning) Codelint ${cached.length}`
        : "$(check) Codelint";
    statusBar.tooltip = `${cached.length} finding(s) (cached) — click for output`;
    output.appendLine(
      `Cache hit: ${filePath} (${cached.length} finding(s)) — skipped clang-tidy`
    );
    return true;
  } catch {
    return false;
  }
}

async function lintActiveFile(fix: boolean): Promise<void> {
  const editor = vscode.window.activeTextEditor;
  if (!editor || !isCppLike(editor.document)) {
    vscode.window.showWarningMessage("Codelint: open a C/C++ file first.");
    return;
  }
  if (editor.document.isDirty) {
    await editor.document.save();
  }
  await runOnPath(editor.document.uri.fsPath, fix, { force: true });
}

async function fixFile(uri?: vscode.Uri): Promise<void> {
  const target =
    uri ??
    vscode.window.activeTextEditor?.document.uri;
  if (!target || target.scheme !== "file") {
    vscode.window.showWarningMessage("Codelint: open a C/C++ file first.");
    return;
  }
  const doc = await vscode.workspace.openTextDocument(target);
  if (doc.isDirty) {
    await doc.save();
  }
  await runOnPath(target.fsPath, true, { force: true });
}

async function runOnPath(
  filePath: string,
  fix: boolean,
  options: { force?: boolean } = {}
): Promise<void> {
  if (linting) {
    runner.cancel();
  }
  linting = true;
  try {
    const config = resolveConfig();
    const configKey = makeConfigKey(config);

    if (!fix && !options.force && cacheEnabled()) {
      try {
        const contentHash = hashFileContent(filePath);
        const cached = lintCache.get(filePath, contentHash, configKey);
        if (cached) {
          diagnostics.setForFile(filePath, cached);
          decorations.refreshVisibleEditors();
          statusBar.text =
            cached.length > 0
              ? `$(warning) Codelint ${cached.length}`
              : "$(check) Codelint";
          statusBar.tooltip = `${cached.length} finding(s) (cached)`;
          output.appendLine(
            `Cache hit: ${filePath} (${cached.length} finding(s)) — skipped clang-tidy`
          );
          return;
        }
      } catch {
        // Fall through to live lint if hashing fails.
      }
    }

    output.appendLine(
      `\n=== ${fix ? "fix" : "lint"} ${filePath} ===\n` +
        `clang-tidy: ${config.clangTidyPath}\n` +
        `plugin: ${config.pluginPath}\n` +
        `compile_commands: ${config.compileCommandsDir ?? "(none)"}\n` +
        `checks: ${config.checks}\n`
    );

    statusBar.text = "$(sync~spin) Codelint";

    if (fix) {
      // Clear old underlines first — --fix rewrites disk and old ranges become ghosts.
      diagnostics.clearUri(vscode.Uri.file(filePath));
      lintCache.invalidate(filePath);
      decorations.clear(vscode.Uri.file(filePath));

      await runner.runFile(config, filePath, { fix: true });

      const open = vscode.workspace.textDocuments.find(
        (d) => path.resolve(d.uri.fsPath) === path.resolve(filePath)
      );
      if (open) {
        await vscode.commands.executeCommand("workbench.action.files.revert");
      }

      // --fix stdout still uses pre-fix locations; never paint those ranges.
      // Re-lint the fixed file for accurate diagnostics/decorations.
      output.appendLine(`\n=== re-lint after fix ${filePath} ===`);
    }

    const result = await runner.runFile(config, filePath, { fix: false });
    const combined = `${result.stdout}\n${result.stderr}`;
    const parsed = parseClangTidyOutput(combined).filter((d) => {
      if (!d.check) {
        return d.severity === "error" || d.severity === "fatal error";
      }
      return d.check.startsWith("codelint") || d.check.includes("codelint");
    });
    // Replace (never accumulate) diagnostics + decorations for this file.
    diagnostics.clearUri(vscode.Uri.file(filePath));
    diagnostics.setForFile(filePath, parsed);
    decorations.refreshVisibleEditors();

    if (cacheEnabled()) {
      try {
        const contentHash = hashFileContent(filePath);
        lintCache.set(filePath, contentHash, configKey, parsed);
      } catch {
        // ignore cache write failures
      }
    }

    output.appendLine(
      `Parsed ${parsed.length} codelint finding(s) for ${filePath}`
    );

    statusBar.text =
      parsed.length > 0
        ? `$(warning) Codelint ${parsed.length}`
        : "$(check) Codelint";
    statusBar.tooltip = `${parsed.length} finding(s) — click for output`;
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    output.appendLine(message);
    output.show(true);
    vscode.window.showErrorMessage(`Codelint: ${message}`);
  } finally {
    linting = false;
  }
}

async function lintWorkspace(): Promise<void> {
  const files = await vscode.workspace.findFiles(CPP_GLOB, "**/node_modules/**");
  if (files.length === 0) {
    vscode.window.showInformationMessage("Codelint: no C/C++ files in workspace.");
    return;
  }

  await vscode.window.withProgress(
    {
      location: vscode.ProgressLocation.Notification,
      title: "Codelint",
      cancellable: true,
    },
    async (progress, token) => {
      let done = 0;
      for (const file of files) {
        if (token.isCancellationRequested) {
          runner.cancel();
          break;
        }
        progress.report({
          message: path.basename(file.fsPath),
          increment: 100 / files.length,
        });
        try {
          await runOnPath(file.fsPath, false);
        } catch (err) {
          const message = err instanceof Error ? err.message : String(err);
          output.appendLine(`skip ${file.fsPath}: ${message}`);
        }
        done += 1;
      }
      vscode.window.showInformationMessage(
        `Codelint: scanned ${done}/${files.length} file(s).`
      );
    }
  );
}
