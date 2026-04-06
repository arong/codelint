#include "lint/checkers/expr_mutation_detector.h"

namespace codelint {
namespace lint {

bool ExprMutationDetector::isVariableModified(const clang::VarDecl* var, const clang::Stmt* scope,
                                              clang::ASTContext* ctx) const {
  if (!var || !scope || !ctx) {
    return false;
  }

  auto& analyzer = getOrCreateScopeAnalyzer(scope, *ctx);
  return analyzer.isMutated(var);
}

bool ExprMutationDetector::isParameterModified(const clang::ParmVarDecl* param,
                                               const clang::FunctionDecl* func,
                                               clang::ASTContext* ctx) const {
  if (!param || !func || !ctx) {
    return false;
  }

  auto& analyzer = param_analyzers_[func];
  if (!analyzer) {
    analyzer.reset(clang::FunctionParmMutationAnalyzer::getFunctionParmMutationAnalyzer(
        *func, *ctx, param_mutation_cache_));
  }

  return analyzer->isMutated(param);
}

clang::ExprMutationAnalyzer&
ExprMutationDetector::getOrCreateScopeAnalyzer(const clang::Stmt* scope,
                                               clang::ASTContext& ctx) const {
  auto& analyzer = scope_analyzers_[scope];
  if (!analyzer) {
    analyzer = std::make_unique<clang::ExprMutationAnalyzer>(const_cast<clang::Stmt&>(*scope), ctx);
  }
  return *analyzer;
}

} // namespace lint
} // namespace codelint
