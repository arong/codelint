import * as crypto from "crypto";
import * as fs from "fs";
import { ParsedDiagnostic } from "./parser";

export interface LintCacheEntry {
  contentHash: string;
  configKey: string;
  parsed: ParsedDiagnostic[];
  checkedAt: number;
}

/**
 * In-memory lint result cache keyed by absolute file path.
 * Avoids re-running clang-tidy when switching back to an unchanged file.
 */
export class LintCache {
  private readonly entries = new Map<string, LintCacheEntry>();
  private readonly maxEntries: number;

  constructor(maxEntries = 64) {
    this.maxEntries = maxEntries;
  }

  clear(): void {
    this.entries.clear();
  }

  invalidate(filePath: string): void {
    this.entries.delete(filePath);
  }

  get(
    filePath: string,
    contentHash: string,
    configKey: string
  ): ParsedDiagnostic[] | undefined {
    const hit = this.entries.get(filePath);
    if (!hit) {
      return undefined;
    }
    if (hit.contentHash !== contentHash || hit.configKey !== configKey) {
      this.entries.delete(filePath);
      return undefined;
    }
    // Refresh LRU order
    this.entries.delete(filePath);
    this.entries.set(filePath, hit);
    return hit.parsed;
  }

  set(
    filePath: string,
    contentHash: string,
    configKey: string,
    parsed: ParsedDiagnostic[]
  ): void {
    if (this.entries.has(filePath)) {
      this.entries.delete(filePath);
    }
    this.entries.set(filePath, {
      contentHash,
      configKey,
      parsed,
      checkedAt: Date.now(),
    });
    while (this.entries.size > this.maxEntries) {
      const oldest = this.entries.keys().next().value;
      if (oldest === undefined) {
        break;
      }
      this.entries.delete(oldest);
    }
  }
}

export function hashFileContent(filePath: string): string {
  const buf = fs.readFileSync(filePath);
  return crypto.createHash("sha1").update(buf).digest("hex");
}

export function makeConfigKey(parts: {
  clangTidyPath: string;
  pluginPath: string;
  compileCommandsDir: string | undefined;
  checks: string;
}): string {
  return [
    parts.clangTidyPath,
    parts.pluginPath,
    parts.compileCommandsDir ?? "",
    parts.checks,
  ].join("|");
}
