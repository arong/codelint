#pragma once

#include <llvm/Config/llvm-config.h>

#ifndef CODELINT_LLVM_VERSION_MAJOR
#define CODELINT_LLVM_VERSION_MAJOR LLVM_VERSION_MAJOR
#endif

// Compatibility layer for codelint across LLVM 15-21+
//
// codelint primarily uses stable clang-tidy plugin APIs (ClangTidyCheck,
// ASTMatchers, FixItHint) which are consistent across LLVM 15-21.
// This header is reserved for future version-specific adaptations.

#if CODELINT_LLVM_VERSION_MAJOR < 16
// LLVM 15 specific adaptations (if any)
#endif
