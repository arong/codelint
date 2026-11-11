import * as path from "path";
import * as vscode from "vscode";

/**
 * Extra editor decorations so Codelint findings are visually obvious:
 * full-token underline + light background wash.
 *
 * Ranges are cached per URI so we can re-paint when an editor becomes visible
 * (setDecorations is lost if applied before the editor exists).
 */
export class DecorationsManager {
  private readonly warningDecoration: vscode.TextEditorDecorationType;
  private readonly errorDecoration: vscode.TextEditorDecorationType;
  private readonly byUri = new Map<string, readonly vscode.Diagnostic[]>();

  constructor() {
    this.warningDecoration = vscode.window.createTextEditorDecorationType({
      // Border is more reliable than CSS text-decoration across themes.
      borderWidth: "0 0 2px 0",
      borderStyle: "solid",
      borderColor: "#E2A336",
      overviewRulerColor: "#E2A336",
      overviewRulerLane: vscode.OverviewRulerLane.Right,
      light: {
        backgroundColor: "rgba(226, 163, 54, 0.18)",
      },
      dark: {
        backgroundColor: "rgba(226, 163, 54, 0.28)",
      },
    });
    this.errorDecoration = vscode.window.createTextEditorDecorationType({
      borderWidth: "0 0 2px 0",
      borderStyle: "solid",
      borderColor: "#F14C4C",
      overviewRulerColor: "#F14C4C",
      overviewRulerLane: vscode.OverviewRulerLane.Right,
      light: {
        backgroundColor: "rgba(241, 76, 76, 0.14)",
      },
      dark: {
        backgroundColor: "rgba(241, 76, 76, 0.25)",
      },
    });
  }

  dispose(): void {
    this.clearAll();
    this.warningDecoration.dispose();
    this.errorDecoration.dispose();
  }

  apply(uri: vscode.Uri, diagnostics: readonly vscode.Diagnostic[]): void {
    this.byUri.set(uriKey(uri), diagnostics);
    this.paintUri(uri, diagnostics);
  }

  clear(uri: vscode.Uri): void {
    this.byUri.delete(uriKey(uri));
    this.paintUri(uri, []);
    // Also wipe every visible editor for this path (guards against URI string drift).
    for (const editor of vscode.window.visibleTextEditors) {
      if (sameFile(editor.document.uri, uri)) {
        this.paintEditor(editor, []);
      }
    }
  }

  /** Drop all cached decorations and clear every visible editor. */
  clearAll(): void {
    this.byUri.clear();
    for (const editor of vscode.window.visibleTextEditors) {
      this.paintEditor(editor, []);
    }
  }

  /** Re-apply cached decorations to currently visible editors. */
  refreshVisibleEditors(): void {
    for (const editor of vscode.window.visibleTextEditors) {
      const diags = this.byUri.get(uriKey(editor.document.uri)) ?? [];
      this.paintEditor(editor, diags);
    }
  }

  private paintUri(uri: vscode.Uri, diagnostics: readonly vscode.Diagnostic[]): void {
    const editors = vscode.window.visibleTextEditors.filter((e) =>
      sameFile(e.document.uri, uri)
    );
    for (const editor of editors) {
      this.paintEditor(editor, diagnostics);
    }
  }

  private paintEditor(
    editor: vscode.TextEditor,
    diagnostics: readonly vscode.Diagnostic[]
  ): void {
    const warnings: vscode.DecorationOptions[] = [];
    const errors: vscode.DecorationOptions[] = [];
    for (const d of diagnostics) {
      // Skip invalid/stale ranges after edits/fixes.
      if (d.range.isEmpty || d.range.start.isAfter(editor.document.lineAt(editor.document.lineCount - 1).range.end)) {
        continue;
      }
      const opt: vscode.DecorationOptions = {
        range: d.range,
        hoverMessage: d.message,
      };
      if (d.severity === vscode.DiagnosticSeverity.Error) {
        errors.push(opt);
      } else {
        warnings.push(opt);
      }
    }
    editor.setDecorations(this.warningDecoration, warnings);
    editor.setDecorations(this.errorDecoration, errors);
  }
}

function uriKey(uri: vscode.Uri): string {
  return path.resolve(uri.fsPath);
}

function sameFile(a: vscode.Uri, b: vscode.Uri): boolean {
  return path.resolve(a.fsPath) === path.resolve(b.fsPath);
}
