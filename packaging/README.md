# codelint Packaging Guide

This directory contains scripts for packaging the codelint clang-tidy plugin.

## Directory Structure

```
packaging/
├── scripts/
│   ├── bundle_libs.sh       # Bundle plugin with dependencies
│   └── codelint-wrapper.sh  # Wrapper script for easy usage
```

## Prerequisites

**Build the plugin**:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=/opt/homebrew/opt/llvm@21/lib/cmake/llvm  # macOS
cmake --build build

# Verify
ls build/lib/*.so
```

## Usage

### Bundle Plugin with Dependencies

```bash
bash packaging/scripts/bundle_libs.sh
```

Creates a portable tarball containing the plugin and required libraries.

## Output

- **Package**: `codelint-VERSION-ARCH.tar.gz`
- **Includes**: `codelint-plugin.so` + dependencies
- **Location**: `package-output/` in project root

## Testing

```bash
cd package-output && tar xzf codelint-*.tar.gz

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

## Troubleshooting

### "Plugin not found"
- Verify build: `ls build/lib/*.so`
- Rebuild: `cmake --build build`

### "LLVM version mismatch"
- Match plugin LLVM version with system clang-tidy
- Check: `clang-tidy --version`

### "Missing libraries"
- Use `bundle_libs.sh` to package dependencies

## License

MIT License (part of codelint project)
