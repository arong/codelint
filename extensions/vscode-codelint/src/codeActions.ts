import * as vscode from "vscode";

/**
 * MVP Quick Fix: offer "Fix file with Codelint" which runs the fix command.
 * Per-diagnostic --export-fixes WorkspaceEdit can be added later.
 */
export class CodelintCodeActionProvider implements vscode.CodeActionProvider {
  static readonly providedCodeActionKinds = [vscode.CodeActionKind.QuickFix];

  provideCodeActions(
    document: vscode.TextDocument,
    _range: vscode.Range | vscode.Selection,
    context: vscode.CodeActionContext
  ): vscode.CodeAction[] {
    const hasCodelint = context.diagnostics.some((d) => d.source === "codelint");
    if (!hasCodelint) {
      return [];
    }
    if (document.languageId !== "cpp" && document.languageId !== "c") {
      return [];
    }

    const action = new vscode.CodeAction(
      "Fix file with Codelint",
      vscode.CodeActionKind.QuickFix
    );
    action.command = {
      command: "codelint.fixActiveFile",
      title: "Fix file with Codelint",
      arguments: [document.uri],
    };
    action.isPreferred = false;
    return [action];
  }
}
