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

// LangOptions::CPlusPlus23 was introduced in LLVM 16
#if CODELINT_LLVM_VERSION_MAJOR >= 16
#define CODELINT_LANGOPTS_IS_CPP23(LangOpts) (LangOpts.CPlusPlus23)
#else
#define CODELINT_LANGOPTS_IS_CPP23(LangOpts) (false)
#endif

// Singleton check can be disabled for CI/release builds
// Define CODELINT_DISABLE_SINGLETON_CHECK to exclude the singleton pattern detection check
#ifdef CODELINT_DISABLE_SINGLETON_CHECK
#else
#endif
