#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/Analyses/ExprMutationAnalyzer.h"
#include "llvm/ADT/DenseMap.h"
#include <memory>

namespace codelint {
namespace lint {

class ExprMutationDetector {
public:
  bool isVariableModified(const clang::VarDecl* var, const clang::Stmt* scope,
                          clang::ASTContext* ctx) const;

  bool isParameterModified(const clang::ParmVarDecl* param, const clang::FunctionDecl* func,
                           clang::ASTContext* ctx) const;

  void clearCache() {
    scope_analyzers_.clear();
    param_analyzers_.clear();
  }

private:
  clang::ExprMutationAnalyzer& getOrCreateScopeAnalyzer(const clang::Stmt* scope,
                                                        clang::ASTContext& ctx) const;

  mutable llvm::DenseMap<const clang::Stmt*, std::unique_ptr<clang::ExprMutationAnalyzer>>
      scope_analyzers_;

  mutable clang::ExprMutationAnalyzer::Memoized param_mutation_cache_;

  mutable llvm::DenseMap<const clang::FunctionDecl*,
                         std::unique_ptr<clang::FunctionParmMutationAnalyzer>>
      param_analyzers_;
};

} // namespace lint
} // namespace codelint
