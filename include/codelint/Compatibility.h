#pragma once

#include <llvm/Config/llvm-config.h>

#ifndef CODELINT_LLVM_VERSION_MAJOR
#define CODELINT_LLVM_VERSION_MAJOR LLVM_VERSION_MAJOR
#endif

#include <clang/AST/Expr.h>
#include <llvm/ADT/STLExtras.h>

// LangOptions::CPlusPlus23 was introduced in LLVM 16
#if CODELINT_LLVM_VERSION_MAJOR >= 16
#define CODELINT_LANGOPTS_IS_CPP23(LangOpts) (LangOpts.CPlusPlus23)
#else
#define CODELINT_LANGOPTS_IS_CPP23(LangOpts) (false)
#endif

// InitListExpr::hasDesignatedInit() was introduced in LLVM 16
#if CODELINT_LLVM_VERSION_MAJOR < 16
namespace clang::tidy::codelint::compat {

inline bool hasDesignatedInit(const InitListExpr* ILE) {
  return llvm::any_of(*ILE, [](const Stmt* S) { return isa<DesignatedInitExpr>(S); });
}

} // namespace clang::tidy::codelint::compat
#else
namespace clang::tidy::codelint::compat {

inline bool hasDesignatedInit(const InitListExpr* ILE) {
  return ILE->hasDesignatedInit();
}

} // namespace clang::tidy::codelint::compat
#endif
