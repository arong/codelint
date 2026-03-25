#!/bin/bash

# Build AppImage for cndy

APPDIR="packaging/AppDir"
LIBDIR="$APPDIR/usr/lib"
BINARY="$APPDIR/usr/bin/cndy"
ARCH=$(uname -m)

echo "Building cndy AppImage for $ARCH..."

# Step 1: Copy binary
echo "Step 1: Copying binary..."
cp build/cndy "$BINARY"

# Step 2: Copy non-system libraries
echo "Step 2: Copying libraries..."
mkdir -p "$LIBDIR"

# Get list of required libraries
LIBS=$(ldd "$BINARY" 2>/dev/null | grep "=>" | grep -v "not found" | awk '{print $3}' | sort -u)

for lib in $LIBS; do
    if [ ! -f "$lib" ]; then
        echo "Warning: Library not found: $lib"
        continue
    fi
    
    libname=$(basename "$lib")
    
    # Skip system libraries that should be on all Linux systems
    if [[ "$libname" == libc.so.* || 
          "$libname" == libm.so.* || 
          "$libname" == libdl.so.* || 
          "$libname" == libpthread.so.* ||
          "$libname" == ld-linux*.so.* ]]; then
        continue
    fi
    
    # Skip LLVM and clang libraries (too large, use system libraries)
    if [[ "$libname" == libclang* || "$libname" == libLLVM* ]]; then
        echo "Skipping (use system): $libname"
        continue
    fi
    
    echo "Copying: $libname"
    cp "$lib" "$LIBDIR/"
done

# Step 3: Copy libraries from /usr/lib/x86_64-linux-gnu if needed
echo "Step 3: Checking for additional libraries..."
for lib in libstdc++.so.6 libgcc_s.so.1; do
    if [ ! -f "$LIBDIR/$lib" ] && [ -f "/usr/lib/x86_64-linux-gnu/$lib" ]; then
        echo "Copying: $lib"
        cp "/usr/lib/x86_64-linux-gnu/$lib" "$LIBDIR/"
    fi
done

# Step 4: Create icon placeholder
echo "Step 4: Creating icon..."
# Create a simple SVG icon
cat > "$APPDIR/cndy.svg" << 'ICON_EOF'
<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
  <rect width="128" height="128" fill="#3B82F6" rx="16"/>
  <text x="64" y="80" text-anchor="middle" font-family="monospace" font-size="48" fill="white" font-weight="bold">C++</text>
  <text x="64" y="110" text-anchor="middle" font-family="sans-serif" font-size="16" fill="white" font-weight="bold">cndy</text>
</svg>
ICON_EOF

# Step 5: Create README
echo "Step 5: Creating README..."
cat > "$APPDIR/README.md" << 'README_EOF'
# cndy - C++ Code Analysis Tool

This is an AppImage distribution of cndy, a C++ code analysis tool.

## Requirements

- Linux (glibc 2.17+)
- LLVM 18 libraries (libclang-18, libLLVM-18)
- Standard system libraries (libc, libm, libstdc++, etc.)

Most modern Linux distributions (Ubuntu 22.04+, Fedora 37+, Debian 12+, etc.) 
have the required system libraries pre-installed.

## Installation of LLVM Dependencies

If you don't have LLVM 18 installed, install it using:

### Ubuntu/Debian:
```bash
sudo apt-get install clang-18 libclang-18-dev
```

### Fedora:
```bash
sudo dnf install clang-devel
```

### Arch Linux:
```bash
sudo pacman -S clang
```

## Usage

Run directly from AppImage:

```bash
./cndy-x86_64.AppImage --help
```

Or extract and install:

```bash
./cndy-x86_64.AppImage --appimage-extract
sudo cp squashfs-root/usr/bin/cndy /usr/local/bin/
```

## Examples

Check initialization issues:
```bash
./cndy-x86_64.AppImage check_init mycode.cpp --fix
```

Find global variables:
```bash
./cndy-x86_64.AppImage find_global mycode.cpp
```

Find singletons:
```bash
./cndy-x86_64.AppImage find_singleton mycode.cpp
```

## Building from Source

Source code: https://github.com/yourusername/cndy
README_EOF

# Step 6: Calculate size
SIZE=$(du -sh "$APPDIR" | cut -f1)
echo ""
echo "AppDir size: $SIZE"
echo ""
echo "Directory structure:"
find "$APPDIR" -type f -o -type l | sed 's|packaging/AppDir||' | sort
echo ""
echo "Note: This AppImage requires system LLVM 18 libraries to be installed."
echo "See README.md for installation instructions."