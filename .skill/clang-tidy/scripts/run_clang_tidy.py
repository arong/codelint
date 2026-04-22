#!/usr/bin/env python3
"""
run_clang_tidy.py - Clang-tidy skill main runner

Provides integrated clang-tidy static analysis with:
- Bundled clang-tidy binary detection
- codelint plugin auto-loading
- Multiple preset configurations (default, strict, security)
- Parallel execution with job control
- compile_commands.json support

USAGE:
    run_clang_tidy.py [OPTIONS] [FILES...]

OPTIONS:
    -p PATH         Path to compile_commands.json directory
    -j N            Number of parallel jobs (default: auto)
    --fix           Apply suggested fixes automatically
    --preset NAME   Use preset config (default, strict, security)
    --checks PATTERN Filter checks (overrides preset)
    --config PATH   Custom .clang-tidy config file
    --load PATH     Load custom clang-tidy plugin
    --list-checks   List all available checks
    --export-fixes  Export fixes to YAML file
    --format-style  Format style for fixes (file, llvm, google)
    -v, --verbose   Show detailed progress

EXAMPLES:
    # Analyze entire project
    run_clang_tidy.py -p build -j 8

    # Use strict preset
    run_clang_tidy.py -p build --preset strict

    # Auto-fix issues
    run_clang_tidy.py -p build --fix

    # Security audit
    run_clang_tidy.py -p build --preset security

    # Analyze specific files
    run_clang_tidy.py -p build src/main.cpp src/utils.cpp

AI ASSISTANT INTEGRATION:
    1. Always use -p compile_commands.json for accurate analysis
    2. Use --fix for auto-fixable warnings
    3. Review Error-level warnings manually
    4. Use --preset security for security audits
"""

import argparse
import os
import subprocess
import sys
import multiprocessing
from pathlib import Path


def get_skill_dir():
    """Get the skill directory containing scripts and configs."""
    script_dir = Path(__file__).parent.resolve()
    # Check if we're in the skill directory structure
    if script_dir.name == "scripts":
        return script_dir.parent
    # Check for bundled skill structure
    bundled = script_dir.parent / "share" / "clang-tidy-skill"
    if bundled.exists():
        return bundled
    return script_dir


def find_clang_tidy():
    """Find clang-tidy binary (bundled or system)."""
    skill_dir = get_skill_dir()
    script_dir = Path(__file__).parent.resolve()

    # Check bundled clang-tidy (multiple platform locations)
    bundled_paths = [
        skill_dir / "bin" / "clang-tidy",
        script_dir.parent / "bin" / "clang-tidy",
        Path.cwd() / "bin" / "clang-tidy",
    ]

    for path in bundled_paths:
        if path.exists():
            return str(path)

    # Check platform-specific bundled paths
    platform = sys.platform
    if platform == "darwin":
        arch = "arm64" if os.uname().machine == "arm64" else "x64"
        bundled_mac = skill_dir / "binaries" / f"darwin-{arch}" / "bin" / "clang-tidy"
        if bundled_mac.exists():
            return str(bundled_mac)
    elif platform.startswith("linux"):
        bundled_linux = skill_dir / "binaries" / "linux-x64" / "bin" / "clang-tidy"
        if bundled_linux.exists():
            return str(bundled_linux)

    # Check system clang-tidy (LLVM 21 priority)
    for version in ["21", "20", "19", "18"]:
        candidates = [
            f"/usr/bin/clang-tidy-{version}",
            f"/usr/lib/llvm-{version}/bin/clang-tidy",
            f"/opt/homebrew/opt/llvm@{version}/bin/clang-tidy",
        ]
        for path in candidates:
            if Path(path).exists():
                return path

    # Default system clang-tidy
    result = subprocess.run(["which", "clang-tidy"], capture_output=True, text=True)
    if result.returncode == 0:
        return result.stdout.strip()

    return None


def find_plugin():
    """Find codelint plugin."""
    skill_dir = get_skill_dir()
    script_dir = Path(__file__).parent.resolve()

    # Check bundled plugin
    plugin_paths = [
        skill_dir / "lib" / "codelint-plugin.so",
        skill_dir / "lib" / "codelint-plugin.dylib",
        script_dir.parent / "lib" / "codelint-plugin.so",
        script_dir.parent / "lib" / "codelint-plugin.dylib",
        Path.cwd() / "build" / "lib" / "codelint-plugin.so",
        Path.cwd() / "build" / "lib" / "codelint-plugin.dylib",
    ]

    for path in plugin_paths:
        if path.exists():
            return str(path)

    return None


def get_preset_config(preset_name):
    """Get preset configuration file path."""
    skill_dir = get_skill_dir()

    preset_paths = [
        skill_dir / "configs" / f".clang-tidy.{preset_name}",
        skill_dir.parent / "configs" / f".clang-tidy.{preset_name}",
    ]

    for path in preset_paths:
        if path.exists():
            return str(path)

    return None


def get_library_path():
    """Get library path for bundled libraries."""
    skill_dir = get_skill_dir()
    script_dir = Path(__file__).parent.resolve()

    lib_paths = [
        skill_dir / "lib",
        script_dir.parent / "lib",
        Path.cwd() / "lib",
    ]

    # Check platform-specific bundled paths
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
        if path.exists() and any(path.glob("*.so")) or any(path.glob("*.dylib")):
            return str(path)

    return None


def build_env():
    """Build environment variables for library loading."""
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


def find_compile_commands(path):
    """Find compile_commands.json file."""
    if path:
        cc_path = Path(path) / "compile_commands.json"
        if cc_path.exists():
            return str(cc_path)
        # Check if path is the file itself
        if Path(path).name == "compile_commands.json":
            return path
        return None

    # Search in common locations
    candidates = [
        Path.cwd() / "compile_commands.json",
        Path.cwd() / "build" / "compile_commands.json",
        Path.cwd() / "cmake-build-debug" / "compile_commands.json",
    ]

    for cc_path in candidates:
        if cc_path.exists():
            return str(cc_path)

    return None


def run_parallel(clang_tidy, files, args, env):
    """Run clang-tidy on multiple files in parallel."""
    num_jobs = args.j or multiprocessing.cpu_count()

    # Build base command
    cmd_base = [clang_tidy]

    plugin = args.plugin_path or find_plugin()
    if plugin and not args.raw:
        cmd_base.extend(["--load", plugin])

    if args.checks:
        cmd_base.extend(["--checks", args.checks])
    elif not args.raw:
        cmd_base.extend(["--checks", "codelint-*"])

    if args.fix:
        cmd_base.append("--fix")

    if args.export_fixes:
        cmd_base.extend(["--export-fixes", args.export_fixes])

    if args.format_style:
        cmd_base.extend(["--format-style", args.format_style])

    # compile_commands.json
    cc_path = find_compile_commands(args.p)
    if cc_path:
        cmd_base.extend(["-p", str(Path(cc_path).parent)])

    # Run files
    import concurrent.futures

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=num_jobs) as executor:
        futures = {executor.submit(run_single, cmd_base + [f], env, args.verbose): f for f in files}
        for future in concurrent.futures.as_completed(futures):
            file = futures[future]
            try:
                result = future.result()
                results.append((file, result))
            except Exception as e:
                print(f"ERROR processing {file}: {e}")
                results.append((file, 1))

    # Summary
    failures = sum(1 for _, r in results if r != 0)
    if failures > 0:
        print(f"\n{failures} files had warnings/errors")
        return 1
    return 0


def run_single(cmd, env, verbose=False):
    """Run clang-tidy on a single file."""
    if verbose:
        print(f"Running: {' '.join(cmd)}")

    result = subprocess.run(cmd, env=env, capture_output=False)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(
        prog="run_clang_tidy.py",
        description="Clang-tidy skill runner with bundled binary support",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument("files", nargs="*", help="Files to check")
    parser.add_argument("-p", dest="compile_path", help="compile_commands.json directory")
    parser.add_argument("-j", type=int, help="Number of parallel jobs")
    parser.add_argument("--fix", action="store_true", help="Apply suggested fixes")
    parser.add_argument("--preset", choices=["default", "strict", "security"],
                        help="Use preset configuration")
    parser.add_argument("--checks", help="Check pattern (overrides preset)")
    parser.add_argument("--config", help="Custom .clang-tidy config file")
    parser.add_argument("--load", dest="plugin_path", help="Custom plugin path")
    parser.add_argument("--list-checks", action="store_true", help="List available checks")
    parser.add_argument("--export-fixes", help="Export fixes to YAML file")
    parser.add_argument("--format-style", default="file", help="Format style for fixes")
    parser.add_argument("--raw", action="store_true", help="Run without plugin")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--version", action="store_true", help="Show version")

    args = parser.parse_args()

    # Find clang-tidy
    clang_tidy = find_clang_tidy()
    if not clang_tidy:
        print("ERROR: clang-tidy not found")
        print("\nInstall options:")
        print("  macOS:  brew install llvm@21")
        print("  Linux:  apt install clang-tidy-21")
        print("  Or use bundled binary from clang-tidy-skill package")
        sys.exit(1)

    # Handle preset config
    if args.preset and not args.config:
        preset_path = get_preset_config(args.preset)
        if preset_path:
            args.config = preset_path
            if args.verbose:
                print(f"Using preset: {args.preset} ({preset_path})")
        else:
            print(f"WARNING: Preset '{args.preset}' not found, using default checks")

    # Apply config file (copy to temp or project root)
    if args.config:
        config_path = Path(args.config)
        if config_path.exists():
            # For now, just warn about config usage
            if args.verbose:
                print(f"Note: Config file {args.config} should be copied to project root as .clang-tidy")

    # Build environment
    env = build_env()

    # Version check
    if args.version:
        result = subprocess.run([clang_tidy, "--version"], env=env)
        plugin = find_plugin()
        if plugin:
            print(f"\nPlugin: {plugin}")
        sys.exit(result.returncode)

    # List checks
    if args.list_checks:
        cmd = [clang_tidy, "--list-checks"]
        plugin = args.plugin_path or find_plugin()
        if plugin:
            cmd.extend(["--load", plugin])
        result = subprocess.run(cmd, env=env)
        sys.exit(result.returncode)

    # Find files to analyze
    files = args.files
    if not files:
        # Find all C++ files in project
        cc_path = find_compile_commands(args.compile_path)
        if cc_path:
            import json
            with open(cc_path) as f:
                data = json.load(f)
                files = [entry["file"] for entry in data]
        else:
            # Search project directory
            project_files = []
            for ext in ["*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hxx"]:
                project_files.extend(Path.cwd().glob(f"src/**/{ext}"))
                project_files.extend(Path.cwd().glob(f"include/**/{ext}"))
            files = [str(f) for f in project_files]

    if not files:
        print("ERROR: No files to analyze")
        print("\nSpecify files or ensure compile_commands.json exists:")
        print("  cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
        sys.exit(1)

    if args.verbose:
        print(f"Analyzing {len(files)} files")
        print(f"clang-tidy: {clang_tidy}")
        plugin = find_plugin()
        if plugin:
            print(f"Plugin: {plugin}")
        lib_path = get_library_path()
        if lib_path:
            print(f"Library path: {lib_path}")

    # Run analysis
    if len(files) > 1 and (args.j or 1) > 1:
        return_code = run_parallel(clang_tidy, files, args, env)
    else:
        # Single file or single job
        cmd = [clang_tidy]

        plugin = args.plugin_path or find_plugin()
        if plugin and not args.raw:
            cmd.extend(["--load", plugin])

        if args.checks:
            cmd.extend(["--checks", args.checks])
        elif not args.raw:
            cmd.extend(["--checks", "codelint-*"])

        if args.fix:
            cmd.append("--fix")

        cc_path = find_compile_commands(args.compile_path)
        if cc_path:
            cmd.extend(["-p", str(Path(cc_path).parent)])

        cmd.extend(files)

        if args.verbose:
            print("Command:", " ".join(cmd))

        result = subprocess.run(cmd, env=env)
        return_code = result.returncode

    sys.exit(return_code)


if __name__ == "__main__":
    main()
