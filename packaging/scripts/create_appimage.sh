#!/bin/bash
# Create AppImage for cndy - Simple shell version

set -e

echo "🚀 Creating cndy AppImage..."

# Check if binary exists
if [ ! -f "build/cndy" ]; then
    echo "❌ Error: build/cndy not found. Please build the project first."
    exit 1
fi

# Check if appimagetool exists
if [ ! -f "packaging/tools/appimagetool" ]; then
    echo "❌ Error: packaging/tools/appimagetool not found."
    echo "Please download appimagetool to packaging/tools/"
    exit 1
fi

# Get version
VERSION=$(git describe --tags 2>/dev/null || echo "dev")
ARCH=$(uname -m)

# Create AppDir structure
APPDIR="packaging/AppDir"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib"

# Copy binary
echo "Copying binary..."
cp build/cndy "$APPDIR/usr/bin/cndy"

# Copy libraries (excluding system libraries)
echo "Copying libraries..."
ldd build/cndy | grep "=>" | while read -r line; do
    lib_path=$(echo "$line" | awk '{print $3}')
    if [ -f "$lib_path" ]; then
        lib_name=$(basename "$lib_path")
        # Skip system libraries
        if [[ "$lib_name" != libc.so.* && \
              "$lib_name" != libm.so.* && \
              "$lib_name" != libdl.so.* && \
              "$lib_name" != libpthread.so.* && \
              "$lib_name" != ld-linux* ]]; then
            echo "Copying: $lib_name"
            cp "$lib_path" "$APPDIR/usr/lib/"
        fi
    fi
done

# Create AppRun
cat > "$APPDIR/AppRun" << 'APP_RUN_EOF'
#!/bin/bash
APPDIR="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"
exec "${APPDIR}/usr/bin/cndy" "$@"
APP_RUN_EOF
chmod +x "$APPDIR/AppRun"

# Create desktop file
cat > "$APPDIR/cndy.desktop" << 'DESKTOP_EOF'
[Desktop Entry]
Name=cndy
Exec=cndy
Icon=cndy
Type=Application
Categories=Development;
Comment=C++ code analysis tool
DESKTOP_EOF

# Create icon
cat > "$APPDIR/cndy.svg" << 'SVG_EOF'
<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
  <rect width="128" height="128" fill="#3B82F6" rx="16"/>
  <text x="64" y="80" text-anchor="middle" font-family="monospace" font-size="48" fill="white" font-weight="bold">C++</text>
  <text x="64" y="110" text-anchor="middle" font-family="sans-serif" font-size="16" fill="white" font-weight="bold">cndy</text>
</svg>
SVG_EOF

# Create README
cat > "$APPDIR/README.md" << 'README_EOF'
# cndy - C++ Code Analysis Tool

This is an AppImage distribution of cndy, a C++ code analysis tool.

## Requirements

- Linux (glibc 2.17+)
- Standard system libraries (libc, libm, libstdc++, etc.)

Note: LLVM 18 libraries are bundled in this AppImage for portability.

## Usage

Run directly from AppImage:

```bash
./cndy-x86_64.AppImage --help
```
README_EOF

echo "✅ AppDir created successfully!"

# Create AppImage
echo "Creating AppImage..."
chmod +x packaging/tools/appimagetool
./packaging/tools/appimagetool --appimage-extract-and-run "$APPDIR" "cndy-$VERSION-$ARCH.AppImage"

echo "🎉 AppImage created successfully!"
echo "File: cndy-$VERSION-$ARCH.AppImage"