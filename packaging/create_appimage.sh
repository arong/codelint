#!/bin/bash

# Create AppImage using appimagetool

set -e

APPDIR="packaging/AppDir"
VERSION=$(git describe --tags 2>/dev/null || echo "dev")
ARCH=$(uname -m)
APPIMAGE_NAME="cndy-${VERSION}-${ARCH}.AppImage"

echo "Creating cndy AppImage..."
echo "Version: $VERSION"
echo "Architecture: $ARCH"
echo ""

# Check if appimagetool exists
if ! command -v appimagetool &> /dev/null; then
    echo "appimagetool not found. Downloading..."
    wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
    APPIMAGETOOL="./appimagetool-x86_64.AppImage"
else
    APPIMAGETOOL="appimagetool"
fi

# Build the AppImage
echo "Building AppImage..."
"$APPIMAGETOOL" "$APPDIR" "$APPIMAGE_NAME"

# Make the AppImage executable
chmod +x "$APPIMAGE_NAME"

echo ""
echo "AppImage created successfully: $APPIMAGE_NAME"
echo "Size: $(du -h "$APPIMAGE_NAME" | cut -f1)"
echo ""
echo "To run:"
echo "  ./$APPIMAGE_NAME --help"
echo ""
echo "To test:"
echo "  ./$APPIMAGE_NAME check_init tests/test_check_init.cpp --fix"
echo ""
echo "Note: This AppImage requires LLVM 18 libraries to be installed on your system."
echo "See $APPDIR/README.md for installation instructions."