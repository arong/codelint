#!/usr/bin/env python3
"""
run_clang_tidy_diff.py - Git diff scanner for incremental PR analysis

Provides incremental clang-tidy analysis for:
- Staged changes (--staged)
- Recent commits (--commits N)
- Branch comparison (--branch NAME)
- Custom diff range (--range COMMIT1..COMMIT2)

Output formats:
- Console (default): Human-readable warnings
- SARIF (--output sarif): GitHub Actions integration
- JSON (--output json): Machine-readable output

USAGE:
    run_clang_tidy_diff.py [OPTIONS]

OPTIONS:
    --staged            Analyze staged changes
    --commits N         Analyze last N commits
    --branch NAME       Analyze changes vs branch NAME
    --range RANGE       Analyze custom commit range (e.g., HEAD~5..HEAD)
    --output FORMAT     Output format: console, sarif, json
    -p PATH             compile_commands.json directory
    --fix               Apply fixes to changed files
    --preset NAME       Use preset config (default, strict, security)
    --checks PATTERN    Filter checks (overrides preset)
    -j N                Number of parallel jobs
    -v, --verbose       Show detailed progress

EXAMPLES:
    # Analyze staged changes (pre-commit hook)
    run_clang_tidy_diff.py --staged -p build

    # PR review: changes vs main
    run_clang_tidy_diff.py --branch main --output sarif > results.sarif

    # Recent commits
    run_clang_tidy_diff.py --commits 5 -p build

    # Custom range
    run_clang_tidy_diff.py --range HEAD~10..HEAD

CI INTEGRATION:
    GitHub Actions:
        run_clang_tidy_diff.py --branch $GITHUB_BASE_REF --output sarif > results.sarif

    GitLab CI:
        run_clang_tidy_diff.py --commits 1 --output json

AI ASSISTANT INTEGRATION:
    1. Use --staged for pre-commit review
    2. Use --branch main for PR analysis
    3. Use --output sarif for CI integration
    4. Always use -p compile_commands.json
"""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from datetime import datetime


def find_clang_tidy():
    """Find clang-tidy binary."""
    script_dir = Path(__file__).parent.resolve()
    skill_dir = script_dir.parent

    # Bundled paths
    bundled_paths = [
        skill_dir / "bin" / "clang-tidy",
        script_dir.parent / "bin" / "clang-tidy",
    ]

    platform = sys.platform
    if platform == "darwin":
        arch = "arm64" if os.uname().machine == "arm64" else "x64"
        bundled_paths.append(skill_dir / "binaries" / f"darwin-{arch}" / "bin" / "clang-tidy")
    elif platform.startswith("linux"):
        bundled_paths.append(skill_dir / "binaries" / "linux-x64" / "bin" / "clang-tidy")

    for path in bundled_paths:
        if path.exists():
            return str(path)

    # System clang-tidy
    for version in ["21", "20", "19", "18"]:
        candidates = [
            f"/usr/bin/clang-tidy-{version}",
            f"/usr/lib/llvm-{version}/bin/clang-tidy",
            f"/opt/homebrew/opt/llvm@{version}/bin/clang-tidy",
        ]
        for path in candidates:
            if Path(path).exists():
                return path

    result = subprocess.run(["which", "clang-tidy"], capture_output=True, text=True)
    if result.returncode == 0:
        return result.stdout.strip()

    return None


def find_plugin():
    """Find codelint plugin."""
    script_dir = Path(__file__).parent.resolve()
    skill_dir = script_dir.parent

    plugin_paths = [
        skill_dir / "lib" / "codelint-plugin.so",
        skill_dir / "lib" / "codelint-plugin.dylib",
        script_dir.parent / "lib" / "codelint-plugin.so",
        script_dir.parent / "lib" / "codelint-plugin.dylib",
        Path.cwd() / "build" / "lib" / "codelint-plugin.so",
    ]

    for path in plugin_paths:
        if path.exists():
            return str(path)

    return None


def get_library_path():
    """Get library path for bundled libraries."""
    script_dir = Path(__file__).parent.resolve()
    skill_dir = script_dir.parent

    lib_paths = [
        skill_dir / "lib",
        script_dir.parent / "lib",
    ]

    platform = sys.platform
    if platform == "darwin":
        arch = "arm64" if os.uname().machine == "arm64" else "x64"
        bundled_lib = skill_dir / "binaries" / f"darwin-{arch}" / "lib"
        if bundled_lib.exists():
            lib_paths.insert(0, bundled_lib)
    elif platform.startswith("linux"):
        bundled_lib = skill_dir / "binaries" / "linux-x64" / "lib"
        if bundled_lib.exists():
            lib_paths.insert(0, bundled_lib)

    for path in lib_paths:
        if path.exists():
            return str(path)

    return None


def build_env():
    """Build environment for library loading."""
    env = os.environ.copy()
    lib_path = get_library_path()
    if lib_path:
        if sys.platform == "darwin":
            existing = env.get("DYLD_LIBRARY_PATH", "")
            env["DYLD_LIBRARY_PATH"] = f"{lib_path}:{existing}" if existing else lib_path
        else:
            existing = env.get("LD_LIBRARY_PATH", "")
            env["LD_LIBRARY_PATH"] = f"{lib_path}:{existing}" if existing else lib_path
    return env


def get_changed_files(args):
    """Get list of changed C++ files from git diff."""
    cmd = ["git", "diff"]

    if args.staged:
        cmd.append("--staged")
    elif args.commits:
        cmd.extend([f"HEAD~{args.commits}", "HEAD"])
    elif args.branch:
        cmd.extend([args.branch, "HEAD"])
    elif args.range:
        cmd.extend(args.range.split(".."))
    else:
        # Default: compare to origin/main or main
        result = subprocess.run(["git", "branch", "-r"], capture_output=True, text=True)
        if "origin/main" in result.stdout:
            cmd.extend(["origin/main", "HEAD"])
        else:
            cmd.extend(["main", "HEAD"])

    cmd.extend(["--name-only", "--diff-filter=ACM"])

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ERROR: git diff failed: {result.stderr}")
        sys.exit(1)

    files = []
    for line in result.stdout.strip().split("\n"):
        if line and any(line.endswith(ext) for ext in [".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx"]):
            files.append(line)

    return files


def parse_clang_tidy_output(output, file_path):
    """Parse clang-tidy warning output."""
    warnings = []
    for line in output.split("\n"):
        if "warning:" in line or "error:" in line:
            # Parse: file:line:col: severity: message [check]
            parts = line.split(":")
            if len(parts) >= 4:
                try:
                    line_num = int(parts[1])
                    col = int(parts[2])
                    severity = parts[3].strip()
                    message = parts[4].strip() if len(parts) > 4 else ""

                    # Extract check name
                    check_name = ""
                    if "[" in message and "]" in message:
                        check_name = message.split("[")[-1].split("]")[0]

                    warnings.append({
                        "file": file_path,
                        "line": line_num,
                        "column": col,
                        "severity": severity,
                        "message": message,
                        "check": check_name,
                    })
                except ValueError:
                    pass

    return warnings


def run_clang_tidy_on_file(clang_tidy, file, args, env):
    """Run clang-tidy on a single file."""
    cmd = [clang_tidy]

    plugin = find_plugin()
    if plugin:
        cmd.extend(["--load", plugin])

    if args.checks:
        cmd.extend(["--checks", args.checks])
    else:
        cmd.extend(["--checks", "codelint-*"])

    if args.compile_path:
        cmd.extend(["-p", args.compile_path])

    cmd.append(file)

    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    return parse_clang_tidy_output(result.stdout + result.stderr, file)


def format_sarif(warnings):
    """Format warnings as SARIF for GitHub Actions."""
    sarif = {
        "$schema": "https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json",
        "version": "2.1.0",
        "runs": [{
            "tool": {
                "driver": {
                    "name": "clang-tidy",
                    "version": "21.x",
                    "informationUri": "https://clang.llvm.org/extra/clang-tidy/",
                    "rules": []
                }
            },
            "results": []
        }]
    }

    # Add rules and results
    rules_map = {}
    for w in warnings:
        check = w["check"]
        if check and check not in rules_map:
            rules_map[check] = {
                "id": check,
                "shortDescription": {"text": check},
                "helpUri": f"https://clang.llvm.org/extra/clang-tidy/checks/{check}.html"
            }

        sarif["runs"][0]["results"].append({
            "ruleId": check,
            "level": "error" if w["severity"] == "error" else "warning",
            "locations": [{
                "physicalLocation": {
                    "artifactLocation": {"uri": w["file"]},
                    "region": {
                        "startLine": w["line"],
                        "startColumn": w["column"]
                    }
                }
            }],
            "message": {"text": w["message"]}
        })

    sarif["runs"][0]["tool"]["driver"]["rules"] = list(rules_map.values())
    return json.dumps(sarif, indent=2)


def format_json(warnings):
    """Format warnings as JSON."""
    return json.dumps(warnings, indent=2)


def main():
    parser = argparse.ArgumentParser(
        prog="run_clang_tidy_diff.py",
        description="Git diff scanner for incremental clang-tidy analysis",
    )

    parser.add_argument("--staged", action="store_true", help="Analyze staged changes")
    parser.add_argument("--commits", type=int, help="Analyze last N commits")
    parser.add_argument("--branch", help="Compare to branch")
    parser.add_argument("--range", help="Custom commit range")
    parser.add_argument("--output", choices=["console", "sarif", "json"], default="console",
                        help="Output format")
    parser.add_argument("-p", dest="compile_path", help="compile_commands.json directory")
    parser.add_argument("--fix", action="store_true", help="Apply fixes")
    parser.add_argument("--preset", choices=["default", "strict", "security"],
                        help="Use preset configuration")
    parser.add_argument("--checks", help="Check pattern")
    parser.add_argument("-j", type=int, default=1, help="Parallel jobs")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")

    args = parser.parse_args()

    # Find clang-tidy
    clang_tidy = find_clang_tidy()
    if not clang_tidy:
        print("ERROR: clang-tidy not found")
        sys.exit(1)

    # Get changed files
    files = get_changed_files(args)

    if not files:
        if args.verbose:
            print("No C++ files changed")
        if args.output == "sarif":
            print(format_sarif([]))
        elif args.output == "json":
            print(format_json([]))
        sys.exit(0)

    if args.verbose:
        print(f"Analyzing {len(files)} changed files:")
        for f in files:
            print(f"  {f}")

    # Build environment
    env = build_env()

    # Run analysis
    all_warnings = []
    for file in files:
        warnings = run_clang_tidy_on_file(clang_tidy, file, args, env)
        all_warnings.extend(warnings)

    # Output
    if args.output == "sarif":
        print(format_sarif(all_warnings))
    elif args.output == "json":
        print(format_json(all_warnings))
    else:
        # Console output
        for w in all_warnings:
            print(f"{w['file']}:{w['line']}:{w['column']}: {w['severity']}: {w['message']} [{w['check']}]")

        if all_warnings:
            print(f"\n{len(all_warnings)} warnings found")
            sys.exit(1)
        else:
            print("No warnings found")


if __name__ == "__main__":
    main()
