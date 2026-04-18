#!/usr/bin/env python3
"""
Self-check codelint source code using the codelint plugin.

IMPORTANT: This script is carefully designed to NOT modify system headers!

Safety measures:
1. NO --fix-errors flag (that would modify system headers)
2. header-filter only matches project files ('codelint/.*')
3. Only checks src/ directory (excludes tests)
4. Two-step process: check first, then fix with confirmation
"""

import argparse
import glob
import os
import subprocess
import sys
from pathlib import Path
from typing import Optional


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

    @classmethod
    def disable(cls):
        """Disable colors for non-terminal output."""
        cls.RED = ''
        cls.GREEN = ''
        cls.YELLOW = ''
        cls.BLUE = ''
        cls.NC = ''


def print_header(title: str):
    """Print a formatted header."""
    print()
    print("=" * 40)
    print(title)
    print("=" * 40)


def print_error(message: str):
    """Print an error message with red color."""
    print(f"{Colors.RED}❌ {message}{Colors.NC}")


def print_success(message: str):
    """Print a success message with green color."""
    print(f"{Colors.GREEN}✅ {message}{Colors.NC}")


def print_warning(message: str):
    """Print a warning message with yellow color."""
    print(f"{Colors.YELLOW}⚠️  {message}{Colors.NC}")


def print_info(message: str):
    """Print an info message."""
    print(f"{Colors.BLUE}ℹ️  {message}{Colors.NC}")


class CodelintSelfCheck:
    """Main class for codelint self-check operations."""

    # Default configuration
    LLVM_BIN = "/opt/homebrew/opt/llvm@21/bin"
    SDK_PATH = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
    PLUGIN_NAME = "codelint-plugin.dylib"
    CHECKS = "codelinit-*"
    FIX_CHECKS = "codelint-init"  # Only auto-fixable checks

    def __init__(self, project_root: Optional[Path] = None, llvm_bin: Optional[str] = None):
        """Initialize with project configuration."""
        self.project_root = project_root or Path(__file__).parent.parent.resolve()
        self.llvm_bin = llvm_bin or self.LLVM_BIN
        self.plugin_path = self.project_root / "build" / "lib" / self.PLUGIN_NAME
        self.compile_db_path = self.project_root / "build" / "compile_commands.json"
        self.src_files = glob.glob(str(self.project_root / "src" / "**" / "*.cpp"), recursive=True)
        self.issues_file = "/tmp/codelint_issues.txt"

    def check_prerequisites(self) -> bool:
        """Check that all required tools and files exist."""
        print_header("Checking Prerequisites")

        # Check LLVM
        if not Path(self.llvm_bin).is_dir():
            print_error(f"LLVM not found at {self.llvm_bin}")
            print("Install with: brew install llvm@21")
            return False
        print_success(f"LLVM found at {self.llvm_bin}")

        clang_tidy = Path(self.llvm_bin) / "clang-tidy"
        if not clang_tidy.is_file():
            print_error(f"clang-tidy not found at {clang_tidy}")
            return False

        clang_format = Path(self.llvm_bin) / "clang-format"
        if not clang_format.is_file():
            print_error(f"clang-format not found at {clang_format}")
            return False

        # Check plugin
        if not self.plugin_path.is_file():
            print_error(f"Plugin not found at {self.plugin_path}")
            print("Build with: cmake --build build --target codelint-plugin")
            return False
        print_success(f"Plugin found at {self.plugin_path}")

        # Check compile_commands.json
        if not self.compile_db_path.is_file():
            print_error("compile_commands.json not found")
            print("Generate with: cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
            return False
        print_success("compile_commands.json found")

        # Check source files
        if not self.src_files:
            print_error("No source files found in src/")
            return False
        print_success(f"Found {len(self.src_files)} source files")

        return True

    def run_check(self) -> int:
        """Run clang-tidy check without modifying files."""
        print_header("Step 1: Checking Source Code")

        print(f"Checking {len(self.src_files)} files in src/...")
        print()
        print_info("SAFETY: NO --fix-errors, header-filter='codelint/.*' only")
        print()

        clang_tidy = Path(self.llvm_bin) / "clang-tidy"

        cmd = [
            str(clang_tidy),
            f"--load={self.plugin_path}",
            "--checks=codelint-*",
            "-p", str(self.project_root / "build"),
            "--header-filter=codelint/.*",
            f"--extra-arg=-isysroot{self.SDK_PATH}",
        ] + self.src_files

        # Run and capture output
        result = subprocess.run(cmd, capture_output=True, text=True)

        # Filter for codelint issues in project source files
        issues = []
        for line in result.stdout.splitlines():
            # Match lines like: /path/to/src/file.cpp:123:45: warning: ... [codelint-...]
            if str(self.project_root) in line and "[codelint-" in line:
                issues.append(line)

        # Write issues to file
        with open(self.issues_file, "w") as f:
            f.write("\n".join(issues))

        # Print issues
        for issue in issues:
            print(issue)

        issue_count = len(issues)

        if issue_count == 0:
            print_success("No codelint issues found!")
            return 0
        else:
            print_warning(f"Found {issue_count} issue(s)")
            return 1

    def run_fix(self, auto_confirm: bool = False) -> bool:
        """Apply fixes to source files."""
        print_header("Step 2: Fixing Issues")

        # Check if there are issues to fix
        if not os.path.exists(self.issues_file):
            print_success("No issues to fix")
            return True

        with open(self.issues_file, "r") as f:
            issues = f.read().strip()

        if not issues:
            print_success("No issues to fix")
            return True

        print("Issues found:")
        print(issues)
        print()

        # Ask for confirmation
        if not auto_confirm:
            try:
                response = input("Do you want to apply fixes? (y/N): ").strip().lower()
                if response not in ("y", "yes"):
                    print_warning("Skipping fixes")
                    return True
            except KeyboardInterrupt:
                print()
                print_warning("Cancelled by user")
                return False

        print("Applying fixes...")
        print_info("SAFETY: NO --fix-errors, only --fix for project files")

        clang_tidy = Path(self.llvm_bin) / "clang-tidy"

        cmd = [
            str(clang_tidy),
            f"--load={self.plugin_path}",
            "--checks=codelint-init",  # Only auto-fixable checks
            "-p", str(self.project_root / "build"),
            "--header-filter=codelint/.*",
            "--fix",
            f"--extra-arg=-isysroot{self.SDK_PATH}",
        ] + self.src_files

        result = subprocess.run(cmd, capture_output=True, text=True)

        # Check for applied fixes
        for line in result.stdout.splitlines():
            if "fix" in line.lower():
                print(line)

        print_success("Fixes applied")

        # Format fixed files
        print("Formatting fixed files...")
        clang_format = Path(self.llvm_bin) / "clang-format"
        subprocess.run([str(clang_format), "-i"] + self.src_files)
        print_success("Formatting complete")

        # Rebuild to verify
        print("Rebuilding plugin...")
        build_result = subprocess.run(
            ["cmake", "--build", str(self.project_root / "build"), "--target", "codelint-plugin"],
            capture_output=True, text=True
        )

        if build_result.returncode != 0:
            print_error("Build failed after fixes")
            print(build_result.stderr)
            return False

        print_success("Rebuild complete")
        return True

    def run(self, auto_fix: bool = False, check_only: bool = False) -> int:
        """Run the full self-check process."""
        print_header("Codelint Self-Check Tool")

        print()
        print_warning("SAFETY NOTICE:")
        print("  This script will NOT modify system headers (LLVM, SDK, etc.)")
        print("  - header-filter restricts to project files only")
        print("  - NO --fix-errors flag (prevents system header modification)")
        print("  - Only src/ directory is checked (tests excluded)")
        print()

        if not self.check_prerequisites():
            return 1

        check_result = self.run_check()

        if check_result != 0 and not check_only:
            if auto_fix:
                if not self.run_fix(auto_confirm=True):
                    return 1
            else:
                if not self.run_fix():
                    return 1

        print_header("Summary")

        if check_result == 0:
            print_success("All checks passed!")
            return 0
        else:
            print_warning("Issues were found and may have been fixed")
            print("Run this script again to verify fixes")
            return 0


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Self-check codelint source code using the codelint plugin",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                   # Check and ask before fixing
  %(prog)s --check-only      # Only check, no fixes
  %(prog)s --auto-fix        # Check and fix without confirmation
  %(prog)s --llvm-bin /path  # Use custom LLVM path

Safety: This script NEVER modifies system headers (LLVM, SDK, etc.)
        """
    )

    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Only check, do not apply fixes"
    )

    parser.add_argument(
        "--auto-fix",
        action="store_true",
        help="Apply fixes without confirmation"
    )

    parser.add_argument(
        "--llvm-bin",
        type=str,
        default=CodelintSelfCheck.LLVM_BIN,
        help=f"Path to LLVM bin directory (default: {CodelintSelfCheck.LLVM_BIN})"
    )

    parser.add_argument(
        "--project-root",
        type=str,
        default=None,
        help="Path to project root (default: auto-detected)"
    )

    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output"
    )

    args = parser.parse_args()

    # Disable colors if requested or if not in terminal
    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    # Initialize and run
    checker = CodelintSelfCheck(
        project_root=Path(args.project_root) if args.project_root else None,
        llvm_bin=args.llvm_bin
    )

    return checker.run(auto_fix=args.auto_fix, check_only=args.check_only)


if __name__ == "__main__":
    sys.exit(main())
