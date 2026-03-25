#!/bin/bash

# Create a portable release package

set -e

VERSION=$(git describe --tags 2>/dev/null || echo "dev")
ARCH=$(uname -m)
RELEASE_DIR="cndy-${VERSION}-${ARCH}"

echo "Creating portable release package for cndy..."
echo "Version: $VERSION"
echo "Architecture: $ARCH"
echo ""

# Clean up previous release
rm -rf "$RELEASE_DIR"

# Create release directory structure
mkdir -p "$RELEASE_DIR/bin"
mkdir -p "$RELEASE_DIR/lib"
mkdir -p "$RELEASE_DIR/docs"

# Copy binary
echo "Copying binary..."
cp build/cndy "$RELEASE_DIR/bin/"

# Copy libraries
echo "Copying libraries..."
BINARY="$RELEASE_DIR/bin/cndy"
LIBS=$(ldd "$BINARY" 2>/dev/null | grep "=>" | grep -v "not found" | awk '{print $3}' | sort -u)

for lib in $LIBS; do
    if [ ! -f "$lib" ]; then
        continue
    fi
    
    libname=$(basename "$lib")
    
    # Skip system libraries
    if [[ "$libname" == libc.so.* || 
          "$libname" == libm.so.* || 
          "$libname" == libdl.so.* || 
          "$libname" == libpthread.so.* ||
          "$libname" == ld-linux*.so.* ||
          "$libname" == libclang* ||
          "$libname" == libLLVM* ]]; then
        continue
    fi
    
    echo "  - $libname"
    cp "$lib" "$RELEASE_DIR/lib/"
done

# Create wrapper script
echo "Creating wrapper script..."
cat > "$RELEASE_DIR/cndy" << 'WRAPPER_EOF'
#!/bin/bash

# cndy wrapper script for portable distribution
# This script sets up the environment and runs cndy

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Add bundled libraries to LD_LIBRARY_PATH
export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH}"

# Run the actual binary
exec "${SCRIPT_DIR}/bin/cndy" "$@"
WRAPPER_EOF

chmod +x "$RELEASE_DIR/cndy"

# Copy README
echo "Creating documentation..."
cat > "$RELEASE_DIR/README.md" << 'README_EOF'
# cndy - C++ Code Analysis Tool (Portable Distribution)

## Requirements

This portable distribution requires the following to be installed on your system:

### Required System Libraries
- libc.so.6 (part of glibc 2.17+)
- libm.so.6 (math library)
- libdl.so.2 (dynamic loading library)
- libpthread (threading library)

These are available on all modern Linux distributions.

### LLVM/Clang Libraries
- libclang-18.so.18 (or compatible version)
- libLLVM-18.so.1 (or compatible version)

You can install these using:

#### Ubuntu/Debian:
```bash
sudo apt-get install clang-18 libclang-18-dev
```

#### Fedora:
```bash
sudo dnf install clang-devel
```

#### Arch Linux:
```bash
sudo pacman -S clang
```

## Installation

1. Extract the archive:
```bash
tar xzf cndy-VERSION-ARCH.tar.gz
cd cndy-VERSION-ARCH
```

2. Optionally, install to /usr/local/bin:
```bash
sudo cp -r * /usr/local/cndy/
sudo ln -s /usr/local/cndy/cndy /usr/local/bin/cndy
```

3. Verify installation:
```bash
./cndy --help
```

## Usage

### Check initialization issues:
```bash
./cndy check_init mycode.cpp
./cndy check_init mycode.cpp --fix
./cndy check_init mycode.cpp --fix --inplace
```

### Find global variables:
```bash
./cndy find_global mycode.cpp
```

### Find singletons:
```bash
./cndy find_singleton mycode.cpp
```

## Troubleshooting

### Error: "libclang-18.so.18: cannot open shared object file"
Install LLVM/Clang as described above.

### Error: "version `GLIBC_2.xx' not found"
Your system is too old. Upgrade to a modern Linux distribution
with glibc 2.17 or later (Ubuntu 14.04+, Debian 8+, Fedora 20+, etc.).

## Building from Source

Source code and build instructions: https://github.com/yourusername/cndy

## License

[Your License Here]
README_EOF

# Create tarball
echo "Creating tarball..."
tar czf "${RELEASE_DIR}.tar.gz" "$RELEASE_DIR"

echo ""
echo "Release package created successfully!"
echo "  File: ${RELEASE_DIR}.tar.gz"
echo "  Size: $(du -h "${RELEASE_DIR}.tar.gz" | cut -f1)"
echo ""
echo "To distribute:"
echo "  Upload ${RELEASE_DIR}.tar.gz to your release page"
echo ""
echo "To test locally:"
echo "  tar xzf ${RELEASE_DIR}.tar.gz"
echo "  cd $RELEASE_DIR"
echo "  ./cndy --help"