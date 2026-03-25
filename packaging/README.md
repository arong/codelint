# Packaging Guide for cndy

This directory contains scripts and resources for creating distributable packages of cndy.

## Available Packaging Methods

### 1. Portable Release (tar.gz)

**Recommended for**: Most users, easy distribution, smaller size

**Created by**: `create_release.sh`

**Size**: ~16MB

**Requirements**: 
- System LLVM 18 libraries (libclang, libLLVM)
- Standard glibc 2.17+ libraries

**Usage**:
```bash
./packaging/create_release.sh
```

This creates a `cndy-VERSION-ARCH.tar.gz` archive containing:
- The cndy binary
- Bundled non-system libraries
- Wrapper script for easy execution
- README with installation instructions

**Testing**:
```bash
tar xzf cndy-dev-x86_64.tar.gz
cd cndy-dev-x86_64
./cndy --help
./cndy check_init tests/test_check_init.cpp --fix
```

### 2. AppImage (if appimagetool is available)

**Created by**: `create_appimage.sh`

**Requirements**: 
- appimagetool (will be auto-downloaded)
- System LLVM 18 libraries

**Usage**:
```bash
./packaging/create_appimage.sh
```

This creates a `cndy-VERSION-ARCH.AppImage` single-file executable.

### 3. AppImage (manual method)

**Created by**: `create_appimage_manual.sh`

**Usage**:
```bash
./packaging/create_appimage_manual.sh
```

This creates an AppImage without requiring the appimagetool binary.

## Building Process

### Quick Start

```bash
# 1. Build cndy
cmake -B build
cmake --build build

# 2. Create release package
./packaging/create_release.sh
```

### Step-by-Step

#### Step 1: Build the binary

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

#### Step 2: Build AppDir structure

```bash
./packaging/build_appimage.sh
```

This creates the `packaging/AppDir` directory with:
- `AppRun` - Execution wrapper
- `cndy.desktop` - Desktop entry
- `cndy.svg` - Application icon
- `README.md` - User documentation
- `usr/bin/cndy` - The main binary
- `usr/lib/` - Bundled libraries

#### Step 3: Create distribution package

Choose one of the following:

**Option A: Portable tarball (Recommended)**
```bash
./packaging/create_release.sh
```

**Option B: AppImage**
```bash
./packaging/create_appimage.sh
```

**Option C: Manual AppImage**
```bash
./packaging/create_appimage_manual.sh
```

## Library Bundling Strategy

The packaging scripts use a smart bundling strategy:

### Bundled Libraries
These are included in the package to ensure compatibility:
- libstdc++.so.6 (C++ standard library)
- libgcc_s.so.1 (GCC runtime)
- libbsd.so.0 (BSD compatibility)
- libedit.so.2 (Line editor)
- libffi.so.8 (Foreign Function Interface)
- libicudata.so.74, libicuuc.so.74 (ICU libraries)
- liblzma.so.5 (XZ compression)
- libmd.so.0 (Message digest)
- libxml2.so.2 (XML parsing)
- libz.so.1 (Compression)
- libzstd.so.1 (Zstandard compression)
- libtinfo.so.6 (Terminal info)

### System Libraries (Not Bundled)
These are expected to be on all target systems:
- libc.so.6 (C standard library)
- libm.so.6 (Math library)
- libdl.so.2 (Dynamic loading)
- libpthread (POSIX threads)
- ld-linux*.so.* (Dynamic linker)

### LLVM Libraries (Not Bundled)
Due to their large size (~150MB), these are NOT bundled:
- libclang-18.so.18
- libLLVM-18.so.1

Users must install LLVM 18 separately. See README.md in the package for installation instructions.

## Distribution

### GitHub Releases

1. Create a new release on GitHub
2. Upload the generated package:
   - For tar.gz: `cndy-dev-x86_64.tar.gz`
   - For AppImage: `cndy-dev-x86_64.AppImage`

### User Installation Instructions

#### From tar.gz:
```bash
# Download
wget https://github.com/yourusername/cndy/releases/download/v1.0.0/cndy-1.0.0-x86_64.tar.gz

# Extract
tar xzf cndy-1.0.0-x86_64.tar.gz
cd cndy-1.0.0-x86_64

# Run
./cndy --help
```

#### From AppImage:
```bash
# Download
wget https://github.com/yourusername/cndy/releases/download/v1.0.0/cndy-1.0.0-x86_64.AppImage

# Make executable
chmod +x cndy-1.0.0-x86_64.AppImage

# Run
./cndy-1.0.0-x86_64.AppImage --help
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake clang-18 libclang-18-dev ninja-build
          wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
          chmod +x appimagetool-x86_64.AppImage
          
      - name: Build cndy
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j$(nproc)
          
      - name: Create release packages
        run: |
          cd packaging
          ./create_release.sh
          
      - name: Upload release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            cndy-*.tar.gz
```

## Troubleshooting

### Build Fails

**Problem**: Library not found during packaging
**Solution**: Install required system libraries:
```bash
sudo apt-get install libicu-dev libxml2-dev libedit-dev libncurses5-dev
```

### AppImage Fails to Run

**Problem**: "libclang-18.so.18: cannot open shared object file"
**Solution**: Install LLVM 18 on the target system:
```bash
sudo apt-get install clang-18 libclang-18-dev
```

**Problem**: "version 'GLIBC_2.xx' not found"
**Solution**: The target system is too old. Minimum glibc version: 2.17

### Size Issues

**Problem**: Package too large
**Solution**: The portable release (tar.gz) is optimized at ~16MB. For even smaller size, consider requiring users to install all dependencies manually and just ship the binary.

## File Structure After Packaging

```
packaging/
├── AppDir/                  # AppImage build directory
│   ├── AppRun               # Execution wrapper
│   ├── cndy.desktop         # Desktop entry
│   ├── cndy.svg             # Icon
│   ├── README.md            # Documentation
│   └── usr/
│       ├── bin/cndy         # Binary
│       └── lib/             # Bundled libraries
├── build_appimage.sh        # Build AppDir structure
├── create_appimage.sh       # Create AppImage (with appimagetool)
├── create_appimage_manual.sh # Create AppImage (manual)
└── create_release.sh        # Create portable tarball

Release output:
├── cndy-dev-x86_64.tar.gz   # Portable release (~16MB)
└── cndy-dev-x86_64.AppImage # AppImage (if created)
```

## Customization

### Changing Version Number

Edit the version detection in create scripts:
```bash
VERSION=$(git describe --tags 2>/dev/null || echo "1.0.0")
```

### Adding More Libraries

Modify the library copying logic in `create_release.sh`:
```bash
if [[ "$libname" == yourlib.so* ]]; then
    echo "  - $libname"
    cp "$lib" "$RELEASE_DIR/lib/"
fi
```

### Creating Icon

Edit the SVG in `build_appimage.sh` or replace with your own icon file.