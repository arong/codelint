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
    readme_content = '''# codelint - C++ Code Analysis Tool

This is an AppImage distribution of codelint, a C++ code analysis tool.

## Requirements

- Linux (glibc 2.17+)
- Standard system libraries (libc, libm, libstdc++, etc.)

Note: LLVM 18 libraries are bundled in this AppImage for portability.

## Usage

Run directly from AppImage:

```bash
./codelint-x86_64.AppImage --help
```

## Examples

Check initialization issues:
```bash
./codelint-x86_64.AppImage check_init mycode.cpp --fix
```

Find global variables:
```bash
./codelint-x86_64.AppImage find_global mycode.cpp
```

Find singletons:
```bash
./codelint-x86_64.AppImage find_singleton mycode.cpp
```
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