import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { DecorationsManager } from "./decorations";
import { ParsedDiagnostic } from "./parser";

export class DiagnosticsManager {
  private readonly collection: vscode.DiagnosticCollection;
  private readonly decorations: DecorationsManager;

  constructor(decorations: DecorationsManager) {
    this.collection = vscode.languages.createDiagnosticCollection("codelint");
    this.decorations = decorations;
  }

  get collectionHandle(): vscode.DiagnosticCollection {
    return this.collection;
  }

  dispose(): void {
    this.collection.dispose();
  }

  clearUri(uri: vscode.Uri): void {
    this.collection.delete(uri);
    this.decorations.clear(uri);
  }

  clearAll(): void {
    this.collection.clear();
  }

  setForFile(filePath: string, parsed: ParsedDiagnostic[]): void {
    const primaryUri = vscode.Uri.file(filePath);
    const primaryKey = canonicalPath(filePath);
    const primaryDiags: vscode.Diagnostic[] = [];
    const otherByUri = new Map<string, vscode.Diagnostic[]>();
    const lineCache = new Map<string, string[]>();

    for (const item of parsed) {
      if (item.severity === "note") {
        continue;
      }
      const itemKey = canonicalPath(item.file);
      const lines = getFileLines(item.file, lineCache);
      const diagnostic = toDiagnostic(item, lines);
      if (itemKey === primaryKey || path.basename(itemKey) === path.basename(primaryKey)) {
        // Always attach findings for the linted file to the editor URI we opened.
        primaryDiags.push(diagnostic);
        continue;
      }
      const list = otherByUri.get(itemKey) ?? [];
      list.push(diagnostic);
      otherByUri.set(itemKey, list);
    }

    this.collection.set(primaryUri, primaryDiags);
    this.decorations.apply(primaryUri, primaryDiags);
    for (const [key, diags] of otherByUri) {
      const uri = vscode.Uri.file(key);
      this.collection.set(uri, diags);
      this.decorations.apply(uri, diags);
    }
  }
}

function toDiagnostic(item: ParsedDiagnostic, lines: string[]): vscode.Diagnostic {
  const severity = toSeverity(item.severity);
  const line = Math.max(0, item.line - 1);
  const col = Math.max(0, item.column - 1);
  const pos = new vscode.Position(line, col);
  const open = vscode.workspace.textDocuments.find(
    (d) => canonicalPath(d.uri.fsPath) === canonicalPath(item.file)
  );
  // Prefer VS Code's word range (covers full identifier reliably).
  const wordRange = open?.getWordRangeAtPosition(pos, /[A-Za-z_][A-Za-z0-9_]*/);
  const range = wordRange ?? expandToTokenRange(lines, line, col);
  const message = item.check ? `${item.message} [${item.check}]` : item.message;
  const diagnostic = new vscode.Diagnostic(range, message, severity);
  diagnostic.source = "codelint";
  diagnostic.code = item.check;
  return diagnostic;
}

/**
 * Expand a clang-tidy caret column into the full identifier / token so the
 * editor underline covers e.g. the whole variable name, not a single character.
 */
function expandToTokenRange(lines: string[], line: number, col: number): vscode.Range {
  if (line < 0 || line >= lines.length) {
    return new vscode.Range(line, col, line, col + 1);
  }
  const text = lines[line];
  if (col >= text.length) {
    return new vscode.Range(line, col, line, col + 1);
  }

  // Prefer C/C++ identifier span (covers most codelint findings).
  let start = col;
  let end = col;
  if (isIdentChar(text[col])) {
    while (start > 0 && isIdentChar(text[start - 1])) {
      start -= 1;
    }
    while (end < text.length && isIdentChar(text[end])) {
      end += 1;
    }
    return new vscode.Range(line, start, line, end);
  }

  // If caret lands on punctuation before a name (`int count` → often on 'c'),
  // or on whitespace, scan forward then backward for an identifier.
  let i = col;
  while (i < text.length && /\s/.test(text[i])) {
    i += 1;
  }
  if (i < text.length && isIdentStart(text[i])) {
    start = i;
    end = i + 1;
    while (end < text.length && isIdentChar(text[end])) {
      end += 1;
    }
    return new vscode.Range(line, start, line, end);
  }

  // Fallback: non-space token around the caret.
  start = col;
  end = col + 1;
  while (start > 0 && !/\s/.test(text[start - 1])) {
    start -= 1;
  }
  while (end < text.length && !/\s/.test(text[end])) {
    end += 1;
  }
  if (end <= start) {
    end = start + 1;
  }
  return new vscode.Range(line, start, line, end);
}

function isIdentStart(ch: string): boolean {
  return /[A-Za-z_]/.test(ch);
}

function isIdentChar(ch: string): boolean {
  return /[A-Za-z0-9_]/.test(ch);
}

function getFileLines(filePath: string, cache: Map<string, string[]>): string[] {
  const key = canonicalPath(filePath);
  const cached = cache.get(key);
  if (cached) {
    return cached;
  }

  const open = vscode.workspace.textDocuments.find(
    (d) => canonicalPath(d.uri.fsPath) === key
  );
  let lines: string[];
  if (open) {
    lines = open.getText().split(/\r?\n/);
  } else {
    try {
      lines = fs.readFileSync(filePath, "utf8").split(/\r?\n/);
    } catch {
      lines = [];
    }
  }
  cache.set(key, lines);
  return lines;
}

function toSeverity(
  severity: ParsedDiagnostic["severity"]
): vscode.DiagnosticSeverity {
  switch (severity) {
    case "fatal error":
    case "error":
      return vscode.DiagnosticSeverity.Error;
    case "warning":
      return vscode.DiagnosticSeverity.Warning;
    case "note":
    default:
      return vscode.DiagnosticSeverity.Information;
  }
}

function canonicalPath(p: string): string {
  try {
    return fs.realpathSync(path.resolve(p));
  } catch {
    return path.resolve(p);
  }
}
