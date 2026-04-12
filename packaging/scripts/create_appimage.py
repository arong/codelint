#!/usr/bin/env python3
"""
Create AppImage for codelint C++ code analysis tool

This script automates the entire AppImage creation process:
1. Builds the AppDir structure
2. Copies the binary and all necessary libraries (including LLVM)
3. Creates required AppImage files (AppRun, desktop, icon, README)
4. Uses appimagetool to create the final AppImage

Requirements:
- build/codelint must exist (build the project first with cmake --build build)
- appimagetool must be available in the packaging/tools directory
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

def get_version():
    """Get version from git tags or return 'dev'"""
    try:
        result = subprocess.run(['git', 'describe', '--tags'],
                              capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "dev"

def create_appdir(binary_path="build/codelint", appdir_path="packaging/AppDir"):
    """Create AppDir structure for codelint AppImage"""

    # Ensure binary exists
    if not os.path.exists(binary_path):
        print(f"Error: {binary_path} not found. Please build the project first.")
        sys.exit(1)

    # Create directories
    os.makedirs(f"{appdir_path}/usr/bin", exist_ok=True)
    os.makedirs(f"{appdir_path}/usr/lib", exist_ok=True)

    # Copy the binary
    print("Copying binary...")
    shutil.copy2(binary_path, f"{appdir_path}/usr/bin/codelint")

    # Get library dependencies using ldd
    print("Analyzing library dependencies...")
    result = subprocess.run(["ldd", binary_path], capture_output=True, text=True)
    libs = []

    for line in result.stdout.split('\n'):
        if "=>" in line and not "not found" in line and "/lib/" in line:
            parts = line.strip().split()
            if len(parts) >= 3:
                lib_path = parts[2]
                if os.path.exists(lib_path):
                    libs.append(lib_path)

    # Libraries to skip (system libraries that should be on all Linux systems)
    skip_patterns = [
        'libc.so', 'libm.so', 'libdl.so', 'libpthread.so',
        'ld-linux', '/lib64/ld-linux'
    ]

    copied_libs = set()
    for lib in libs:
        lib_name = os.path.basename(lib)

        # Skip system libraries
        skip = False
        for skip_pattern in skip_patterns:
            if skip_pattern in lib_name:
                skip = True
                break
        if skip:
            continue

        # Copy library
        if lib not in copied_libs:
            print(f"Copying: {lib_name}")
            shutil.copy2(lib, f"{appdir_path}/usr/lib/{lib_name}")
            copied_libs.add(lib)

    # Create AppRun script
    print("Creating AppRun...")
    apprun_content = '''#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"
exec "${APPDIR}/usr/bin/codelint" "$@"
'''
    with open(f"{appdir_path}/AppRun", 'w') as f:
        f.write(apprun_content)
    os.chmod(f"{appdir_path}/AppRun", 0o755)

    # Create desktop file
    print("Creating desktop file...")
    desktop_content = '''[Desktop Entry]
Name=codelint
Exec=codelint
Icon=codelint
Type=Application
Categories=Development;
Comment=C++ code analysis tool
'''
    with open(f"{appdir_path}/codelint.desktop", 'w') as f:
        f.write(desktop_content)

    # Create icon
    print("Creating icon...")
    icon_content = '''<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
  <rect width="128" height="128" fill="#3B82F6" rx="16"/>
  <text x="64" y="80" text-anchor="middle" font-family="monospace" font-size="48" fill="white" font-weight="bold">C++</text>
  <text x="64" y="110" text-anchor="middle" font-family="sans-serif" font-size="16" fill="white" font-weight="bold">codelint</text>
</svg>
'''
    with open(f"{appdir_path}/codelint.svg", 'w') as f:
        f.write(icon_content)

    # Create README
    print("Creating README...")
    readme_content = '''# Codelint - Offline clang-tidy Plugin Package

This package contains clang-tidy with the codelint plugin and all required libraries
for offline deployment on Ubuntu 22.04.

## Contents

- `bin/clang-tidy` - clang-tidy binary (LLVM 21)
- `bin/codelint` - Wrapper script that auto-loads the plugin
- `lib/codelint-plugin.so` - Codelint clang-tidy plugin
- `lib/*.so` - Required LLVM/Clang libraries

## Requirements

- Ubuntu 22.04 (or compatible Linux distribution)
- No LLVM installation required
- No network access required

## Usage

### Quick Start (Recommended)

```bash
# Using the wrapper (auto-loads plugin)
./bin/codelint your_file.cpp

# With compilation database
./bin/codelint -p build/compile_commands.json src/*.cpp

# Apply fixes automatically
./bin/codelint --fix your_file.cpp
```

### Learn What Codelint Does

```bash
# Show detailed functionality
./bin/codelint --describe
```

### Manual Usage

```bash
# Direct clang-tidy invocation
./bin/clang-tidy \
    --load=lib/codelint-plugin.so \
    --checks='codelint-*' \
    your_file.cpp

# Skip plugin loading
./bin/codelint --raw --checks='modernize-*' your_file.cpp
```

## Available Checks

| Check | Purpose | Auto-fix |
|-------|---------|----------|
| `codelint-init` | Variable initialization style | Yes |
| `codelint-global` | Global variable detection | No |
| `codelint-singleton` | Meyer's Singleton pattern | No |

## codelint-init Check Details

### What It Detects and Fixes

| Issue | Before | Auto-fix? | After |
|-------|--------|-----------|-------|
| Uninitialized variable | `int x;` | Yes | `int x{};` |
| Uninitialized array | `int arr[10];` | Yes | `int arr[10]{};` |
| Equals initialization | `int x = 5;` | Yes | `int x{5};` |
| = {} syntax | `int x = {};` | Yes | `int x{};` |
| Unsigned missing U | `unsigned u = 1;` | Yes | `unsigned u{1U};` |
| **Integer to bool** | `bool b = 1;` | **Error** | Manual fix required |
| **Float to int** | `int x = 3.14;` | Warn | Manual fix required |
| **Constructor members** | `Widget() {}` | Warn | Add to initializer list |

### Multiple Declarators

```cpp
// Before: Multiple variables on one line
int a, b, c;

// After: Each variable fixed separately
int a{}, b{}, c{};
```

### Smart Skip List

Codelint skips these cases (intentionally not modified):

| Category | Example | Reason |
|----------|---------|--------|
| For loops | `for (int i = 0; ...)` | C-style idiom |
| Catch blocks | `catch (int e)` | Exception handling |
| Macro definitions | Inside `#define` | Cannot modify |
| Auto types | `auto x = func()` | Type deduction |
| Extern | `extern int x;` | Definition elsewhere |
| Type widening | `float f = 5` | Safe conversion |

## Tips for AI Assistants

1. **Always use `--fix`** for auto-fixable issues
2. **Manually review Error-level warnings** (int→bool)
3. **Check constructor initializer lists** for member warnings
4. **Use `-p compile_commands.json`** for accurate analysis

## Troubleshooting

If you see "library not found" errors:

```bash
# Verify all libraries are present
ldd bin/clang-tidy | grep "not found"

# Check plugin dependencies
ldd lib/codelint-plugin.so | grep "not found"
```

## Version

Package version: ${VERSION}
LLVM version: ${LLVM_VERSION}
'''
    with open(f"{appdir_path}/README.md", 'w') as f:
        f.write(readme_content)

    print(f"✅ AppDir created successfully at {appdir_path}")
    return appdir_path

def create_appimage(appdir_path, output_dir="."):
    """Create AppImage using appimagetool from tools directory"""
    version = get_version()
    arch = subprocess.check_output(["uname", "-m"]).decode().strip()
    appimage_name = f"codelint-{version}-{arch}.AppImage"
    appimage_path = os.path.join(output_dir, appimage_name)

    print(f"Creating AppImage: {appimage_path}")

    # Use appimagetool from tools directory
    appimagetool_path = "packaging/tools/appimagetool"
    if not os.path.exists(appimagetool_path):
        print(f"Error: {appimagetool_path} not found!")
        print("Please download appimagetool to packaging/tools/")
        sys.exit(1)

    # Run appimagetool with --appimage-extract-and-run to avoid FUSE dependency
    try:
        subprocess.run([
            appimagetool_path,
            "--appimage-extract-and-run",
            appdir_path,
            appimage_path
        ], check=True)
        print(f"✅ AppImage created successfully: {appimage_path}")
        return appimage_path
    except subprocess.CalledProcessError as e:
        print(f"❌ Error creating AppImage: {e}")
        sys.exit(1)

def main():
    """Main function to create AppImage"""
    print("🚀 Creating codelint AppImage...")

    # Step 1: Create AppDir
    appdir = create_appdir()

    # Step 2: Create AppImage
    appimage = create_appimage(appdir)

    if appimage:
        size_mb = os.path.getsize(appimage) / (1024 * 1024)
        print(f"\n🎉 AppImage created successfully!")
        print(f"📁 File: {appimage}")
        print(f"📏 Size: {size_mb:.1f} MB")
        print(f"\n🧪 To test:")
        print(f"  ./{os.path.basename(appimage)} --help")
        print(f"  ./{os.path.basename(appimage)} check_init tests/test_check_init.cpp")
    else:
        print("\n❌ Failed to create AppImage")
        sys.exit(1)

if __name__ == "__main__":
    main()
