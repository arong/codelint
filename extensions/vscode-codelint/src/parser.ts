export interface ParsedDiagnostic {
  file: string;
  line: number;
  column: number;
  severity: "error" | "warning" | "note" | "fatal error";
  message: string;
  check?: string;
}

/** clang-tidy: FILE:LINE:COL: severity: MESSAGE [check-name] */
const DIAG_RE =
  /^(.*?):(\d+):(\d+):\s+(fatal error|error|warning|note):\s+(.*?)(?:\s+\[([^\]]+)\])?\s*$/;

export function parseClangTidyOutput(output: string): ParsedDiagnostic[] {
  const results: ParsedDiagnostic[] = [];
  for (const rawLine of output.split(/\r?\n/)) {
    const line = rawLine.trimEnd();
    if (!line) {
      continue;
    }
    const match = DIAG_RE.exec(line);
    if (!match) {
      continue;
    }
    const [, file, lineStr, colStr, severity, message, check] = match;
    results.push({
      file,
      line: Number(lineStr),
      column: Number(colStr),
      severity: severity as ParsedDiagnostic["severity"],
      message: message.trim(),
      check,
    });
  }
  return results;
}
