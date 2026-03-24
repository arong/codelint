# codelint AppImage Packaging Guide

This directory contains all the tools and scripts needed to create portable AppImage packages for the codelint C++ code analysis tool.

## Directory Structure

```
packaging/
├── scripts/              # Packaging scripts
│   └── create_appimage.py    # AppImage creation script
├── tools/                # External tools
│   └── appimagetool          # AppImage creation tool
└── AppDir/               # Temporary AppDir structure (created during build)
```

## Prerequisites

1. **Build the plugin**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm  # macOS
   cmake --build build

   # Verify
   ls build/lib/*.so
   ```

## Usage

```bash
python3 packaging/scripts/create_appimage.py
```

Creates a portable plugin package. See "Installation" below for usage with clang-tidy.

This will create a portable package containing the codelint clang-tidy plugin.

## Output

- **Package**: `codelint-VERSION-ARCH.tar.gz` (or .AppImage)
- **Includes**: `codelint-plugin.so` (+ optional LLVM libs)
- **Size**: Plugin-only ~2MB, with LLVM ~60MB+
- **Location**: `package-output/` in project root

## Features

✅ **Plugin** (`*.so`) for clang-tidy
✅ **Flexible**: Plugin-only or bundle LLVM
✅ **Portable**: Cross-distribution compatible
✅ **Easy**: Drop-in installation or `--load` flag

## Testing

```bash
cd package-output &&tar xzf codelint-*.tar.gz

# Test plugin
clang-tidy --load=lib/codelint-plugin.so --checks='codelint-*' --list-checks

# Test on code
cat > /tmp/test.cpp << 'EOF'
void test() { int x; }
EOF
clang-tidy --load=lib/codelint-plugin.so --checks='codelint-init' /tmp/test.cpp
```

## Installation

**System (sudo)**:
```bash
sudo cp lib/codelint-plugin.so /usr/local/lib/clang-tidy/
```

**User (no sudo)**:
```bash
cp lib/codelint-plugin.so ~/.local/lib/clang-tidy/
clang-tidy --load=~/.local/lib/clang-tidy/codelint-plugin.so --checks='codelint-*' src/*.cpp
```

## Reusing

1. **Rebuild**: `cmake --build build --clean-first`
2. **Package**: `python3 packaging/scripts/create_appimage.py`
3. **Automatic**: Detects version, bundles deps

## Customization

### Version Detection
Edit `get_version()` in `create_appimage.py`.

### Bundle LLVM
Set `BUNDLE_LLVM=true` in `create_appimage.py`.

### Library Filtering
Modify library copying logic to customize which libs are bundled.

### Branding
Update app icon in the script.

## Troubleshooting

### "Plugin not found"
- Verify build: `ls build/lib/*.so`
- Rebuild: `cmake --build build`

### "LLVM version mismatch"
- Match plugin LLVM version with system clang-tidy
- Check: `clang-tidy --version`

### "Missing libraries"
- Plugin-only: System libs handled automatically
- Bundle: Script auto-detects non-system libs

## License

MIT License (part of codelint project)
