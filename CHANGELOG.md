# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Auto-fix support for `codelint-strict-bool-condition`: integer conditions get `!= 0`, pointer conditions get `!= nullptr`
- macOS build artifacts in release workflow
- GitHub Issue and Pull Request templates
- MIT License file
- CHANGELOG.md

## [1.0.0] - 2026-04-16

### Added
- `codelint-init` check: uninitialized variables, dangerous int→bool conversions, narrowing detection with auto-fix
- `codelint-lint-code` check: `=` → `{}` style conversion, unsigned suffix `U`/`UL` with auto-fix
- `codelint-strict-bool-condition` check: bool-only condition enforcement
- `codelint-signed-to-unsigned-return` check: POSIX signed return → unsigned detection
- `codelint-global` check: global variable detection
- `codelint-global-const-string` check: global const string → constexpr suggestion
- `codelint-singleton` check: Meyer's Singleton pattern detection
- Comprehensive test suite with 4-phase regression testing
- GitHub Actions CI/CD with automated releases
- clang-tidy skill package with 4 configuration presets (codelint, default, strict, security)
- Python wrapper script and git diff analysis tools
- Bilingual documentation (English and Chinese)

### Changed
- Migrated from standalone CLI tool to clang-tidy plugin architecture for better ecosystem integration

### Fixed
- Initializer list constructor handling to avoid changing constructor semantics
- Exception handling and macro edge cases in initialization checks
- False positives in auto type brace initialization
