#!/bin/bash

# Manual AppImage creation without appimagetool
# This creates a self-extracting AppImage

set -e

APPDIR="packaging/AppDir"
VERSION=$(git describe --tags 2>/dev/null || echo "dev")
ARCH=$(uname -m)
APPIMAGE_NAME="cndy-${VERSION}-${ARCH}.AppImage"

echo "Creating cndy AppImage (manual method)..."
echo "Version: $VERSION"
echo "Architecture: $ARCH"
echo ""

# Create the AppImage structure
OUTPUT_DIR="AppImage_build"
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Create the main AppImage file
echo "Creating AppImage file..."

# Header for AppImage
cat > "$APPIMAGE_NAME" << 'APPIMAGE_HEADER'
#!/bin/bash
# AppImage self-extracting wrapper
APPIMAGE_EXTRACT_AND_RUN=1
APPIMAGE_TYPE=1

# Self-extract
SELF=$(readlink -f "$0")
if [ -z "$APPIMAGE" ]; then
    export APPIMAGE="$SELF"
fi
export APPDIR="${APPIMAGE%\.AppImage}"

case "$1" in
    --appimage-extract)
        echo "Extracting AppImage..."
        # Get the offset of the archive
        OFFSET=$(awk 'BEGIN{print length($0)}' "$SELF" 2>/dev/null || grep -abo '^__ARCHIVE_BELOW__$' "$SELF" | tail -n1 | cut -d: -f1)
        # Add some padding
        OFFSET=$((OFFSET + 8))
        
        # Extract
        tail -c +$OFFSET "$SELF" | tar xzf -
        
        echo "Extracted to squashfs-root/"
        ;;
    --appimage-offset)
        # Print the offset of the archive
        OFFSET=$(awk 'BEGIN{print length($0)}' "$SELF" 2>/dev/null || grep -abo '^__ARCHIVE_BELOW__$' "$SELF" | tail -n1 | cut -d: -f1)
        OFFSET=$((OFFSET + 8))
        echo $OFFSET
        ;;
    --appimage-version)
        echo "AppImage Type 1"
        ;;
    *)
        # Run the AppImage
        if [ ! -d "$APPDIR" ]; then
            # Create a temporary mount point
            TMPDIR=$(mktemp -d)
            mount -o loop "$SELF" "$TMPDIR" 2>/dev/null || {
                # Fallback to extraction
                OFFSET=$(awk 'BEGIN{print length($0)}' "$SELF" 2>/dev/null || grep -abo '^__ARCHIVE_BELOW__$' "$SELF" | tail -n1 | cut -d: -f1)
                OFFSET=$((OFFSET + 8))
                mkdir -p "$APPDIR"
                tail -c +$OFFSET "$SELF" | tar xzf -C "$APPDIR"
            }
        fi
        
        # Run the application
        if [ -d "$APPDIR" ]; then
            exec "$APPDIR/AppRun" "$@"
        elif [ -d "$TMPDIR" ]; then
            exec "$TMPDIR/AppRun" "$@"
        fi
        ;;
esac
exit 0

__ARCHIVE_BELOW__
APPIMAGE_HEADER

# Append the tar.gz archive
echo "Creating archive..."
cd "$APPDIR"
tar czf - * | dd of=../../../"$APPIMAGE_NAME" oflag=append conv=notrunc 2>/dev/null
cd ../..

# Make it executable
chmod +x "$APPIMAGE_NAME"

echo ""
echo "AppImage created: $APPIMAGE_NAME"
echo "Size: $(du -h "$APPIMAGE_NAME" | cut -f1)"
echo ""
echo "To run:"
echo "  ./$APPIMAGE_NAME --help"
echo ""
echo "Note: This AppImage requires LLVM 18 libraries to be installed on your system."