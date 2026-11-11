import { spawn, ChildProcessWithoutNullStreams } from "child_process";
import * as vscode from "vscode";
import { CodelintResolvedConfig } from "./config";

export interface RunResult {
  stdout: string;
  stderr: string;
  code: number | null;
  command: string[];
}

export class CodelintRunner {
  private current: ChildProcessWithoutNullStreams | undefined;
  private readonly output: vscode.OutputChannel;

  constructor(output: vscode.OutputChannel) {
    this.output = output;
  }

  cancel(): void {
    if (this.current && !this.current.killed) {
      this.current.kill();
      this.current = undefined;
    }
  }

  async runFile(
    config: CodelintResolvedConfig,
    filePath: string,
    options: { fix?: boolean } = {}
  ): Promise<RunResult> {
    this.cancel();

    const args = [
      `--load=${config.pluginPath}`,
      // Force only codelint checks (ignore ambient .clang-tidy extras).
      `--checks=-*,${config.checks}`,
      // Quiet clang-tidy chatter so Problems parsing stays clean.
      `--quiet`,
    ];
    if (config.compileCommandsDir) {
      args.push("-p", config.compileCommandsDir);
    }
    if (options.fix) {
      args.push("--fix");
    }
    args.push(filePath);
    // Fallback compile flags when the file is not in compile_commands.json.
    args.push("--", "-std=c++17", "-xc++");

    return this.spawn(config.clangTidyPath, args);
  }

  private spawn(command: string, args: string[]): Promise<RunResult> {
    const full = [command, ...args];
    this.output.appendLine(`$ ${full.map(shellQuote).join(" ")}`);

    return new Promise((resolve, reject) => {
      let stdout = "";
      let stderr = "";
      const child = spawn(command, args, {
        env: process.env,
      });
      this.current = child;

      child.stdout.on("data", (chunk: Buffer) => {
        const text = chunk.toString();
        stdout += text;
        this.output.append(text);
      });
      child.stderr.on("data", (chunk: Buffer) => {
        const text = chunk.toString();
        stderr += text;
        this.output.append(text);
      });
      child.on("error", (err) => {
        if (this.current === child) {
          this.current = undefined;
        }
        reject(err);
      });
      child.on("close", (code) => {
        if (this.current === child) {
          this.current = undefined;
        }
        this.output.appendLine(`\n[exit ${code ?? "null"}]`);
        resolve({ stdout, stderr, code, command: full });
      });
    });
  }
}

function shellQuote(value: string): string {
  if (/^[A-Za-z0-9_./:@+=,-]+$/.test(value)) {
    return value;
  }
  return `'${value.replace(/'/g, `'\\''`)}'`;
}
