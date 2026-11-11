import * as fs from "fs";
import * as os from "os";
import * as path from "path";
import * as vscode from "vscode";

export interface CodelintResolvedConfig {
  clangTidyPath: string;
  pluginPath: string;
  compileCommandsDir: string | undefined;
  checks: string;
  runOnSave: boolean;
  fixOnSave: boolean;
}

function exists(filePath: string): boolean {
  try {
    return fs.existsSync(filePath);
  } catch {
    return false;
  }
}

function workspaceFolders(): readonly vscode.WorkspaceFolder[] {
  return vscode.workspace.workspaceFolders ?? [];
}

function findClangTidy(configured: string): string | undefined {
  if (configured && exists(configured)) {
    return configured;
  }

  const home = os.homedir();
  const versions = ["21", "20", "19", "18", "15"];
  const candidates: string[] = [];

  for (const version of versions) {
    candidates.push(
      `/opt/homebrew/opt/llvm@${version}/bin/clang-tidy`,
      `/usr/local/opt/llvm@${version}/bin/clang-tidy`,
      `/usr/lib/llvm-${version}/bin/clang-tidy`,
      `/usr/bin/clang-tidy-${version}`,
      path.join(home, `.local/llvm-${version}/bin/clang-tidy`)
    );
  }
  candidates.push("clang-tidy");

  for (const candidate of candidates) {
    if (candidate === "clang-tidy") {
      // Leave bare name for PATH resolution by spawn
      continue;
    }
    if (exists(candidate)) {
      return candidate;
    }
  }

  return "clang-tidy";
}

function pluginCandidates(folder: string): string[] {
  const names = [
    "codelint-core.dylib",
    "codelint-core.so",
    "codelint-plugin.dylib",
    "codelint-plugin.so",
  ];
  const dirs = [
    path.join(folder, "build", "lib"),
    path.join(folder, "lib"),
    path.join(folder, "lib", "clang-tidy"),
    path.join(folder, "build", "lib", "clang-tidy"),
  ];
  const out: string[] = [];
  for (const dir of dirs) {
    for (const name of names) {
      out.push(path.join(dir, name));
    }
  }
  return out;
}

function findPlugin(configured: string): string | undefined {
  if (configured && exists(configured)) {
    return configured;
  }

  for (const folder of workspaceFolders()) {
    for (const candidate of pluginCandidates(folder.uri.fsPath)) {
      if (exists(candidate)) {
        return candidate;
      }
    }
  }
  return undefined;
}

function findCompileCommandsDir(configured: string): string | undefined {
  if (configured) {
    const resolved = path.isAbsolute(configured)
      ? configured
      : path.join(workspaceFolders()[0]?.uri.fsPath ?? "", configured);
    if (exists(path.join(resolved, "compile_commands.json"))) {
      return resolved;
    }
    if (exists(resolved) && path.basename(resolved) === "compile_commands.json") {
      return path.dirname(resolved);
    }
  }

  const searchNames = ["build", "cmake-build-debug", "cmake-build-release", "out", "."];
  for (const folder of workspaceFolders()) {
    for (const name of searchNames) {
      const dir = path.join(folder.uri.fsPath, name);
      if (exists(path.join(dir, "compile_commands.json"))) {
        return dir;
      }
    }
  }
  return undefined;
}

export function resolveConfig(): CodelintResolvedConfig {
  const cfg = vscode.workspace.getConfiguration("codelint");
  const clangTidyPath = findClangTidy(cfg.get<string>("clangTidyPath", "").trim());
  const pluginPath = findPlugin(cfg.get<string>("pluginPath", "").trim());
  const compileCommandsDir = findCompileCommandsDir(
    cfg.get<string>("compileCommandsDir", "").trim()
  );

  if (!clangTidyPath) {
    throw new Error(
      "clang-tidy not found. Set codelint.clangTidyPath or install LLVM (e.g. brew install llvm@21)."
    );
  }
  if (!pluginPath) {
    throw new Error(
      "codelint-core plugin not found. Build the project (cmake --build build) or set codelint.pluginPath."
    );
  }

  return {
    clangTidyPath,
    pluginPath,
    compileCommandsDir,
    checks: cfg.get<string>("checks", "codelint-*") || "codelint-*",
    runOnSave: cfg.get<boolean>("runOnSave", true),
    fixOnSave: cfg.get<boolean>("fixOnSave", false),
  };
}
