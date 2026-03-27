# cndy AppImage Packaging Guide

This directory contains all the tools and scripts needed to create portable AppImage packages for the cndy C++ code analysis tool.

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

1. **Build the project first**:
   ```bash
   cmake -B build
   cmake --build build
   ```

2. **Verify binary exists**:
   ```bash
   ls -la build/cndy
   ```

## Usage

```bash
cd /path/to/cndy
python3 packaging/scripts/create_appimage.py
```

## Output

Creates:
- **AppImage file**: `cndy-VERSION-ARCH.AppImage` (e.g., `cndy-dev-x86_64.AppImage`)
- **Size**: ~63MB (includes bundled LLVM 18 libraries)
- **Location**: Project root directory

## Features

✅ **Fully Portable**: Includes all necessary libraries (LLVM 18, libstdc++, etc.)
✅ **No Dependencies**: Users don't need to install LLVM or other libraries
✅ **FUSE-Free**: Uses `--appimage-extract-and-run` mode, works without FUSE
✅ **Cross-Distribution**: Works on Ubuntu, Fedora, Debian, Arch, etc.
✅ **Easy Distribution**: Single executable file

## Testing

After creation, test the AppImage:
```bash
./cndy-dev-x86_64.AppImage --help
./cndy-dev-x86_64.AppImage check_init tests/test_check_init.cpp
```

## Reusing for Future Releases

To create AppImage for new versions:

1. **Build your updated binary**:
   ```bash
   cmake --build build --clean-first
   ```

2. **Run the packaging script**:
   ```bash
   python3 packaging/scripts/create_appimage.py
   ```

3. **The script will automatically**:
   - Detect git version tags
   - Create appropriate AppImage filename
   - Bundle all current dependencies

## Customization

### Changing Version Detection
Edit the `get_version()` function in `create_appimage.py` to customize version naming.

### Adding More Libraries
Modify the library copying logic in both scripts to include/exclude specific libraries.

### Updating Icon
Replace the SVG content in the scripts or modify the `cndy.svg` generation section.

## Troubleshooting

### "Binary not found"
- Ensure you've built the project with `cmake --build build`
- Verify `build/cndy` exists

### "appimagetool not found"
- Ensure `packaging/tools/appimagetool` exists
- Download from: https://github.com/AppImage/AppImageKit/releases

### Library Issues
- The scripts automatically detect and bundle non-system libraries
- System libraries (libc, libm, etc.) are expected to be on target systems

## License

The packaging scripts are part of the cndy project and follow the same license terms.