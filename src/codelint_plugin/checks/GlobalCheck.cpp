#include "codelint/checks/GlobalCheck.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>

namespace clang::tidy {
namespace codelint {

void GlobalCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (!Finder) {
    return;
  }

  Finder->addMatcher(
      ast_matchers::varDecl(
          ast_matchers::hasGlobalStorage(), ast_matchers::unless(ast_matchers::isStaticLocal()),
          ast_matchers::unless(ast_matchers::parmVarDecl()),
          ast_matchers::unless(ast_matchers::hasAncestor(ast_matchers::recordDecl())))
          .bind("globalVar"),
      this);
}

void GlobalCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("globalVar");
  if (!VD || VD->isImplicit() || VD->getName().empty()) {
    return;
  }

  if (VD->isConstexpr()) {
    return;
  }

  if (VD->getStorageClass() == clang::SC_Extern && !VD->hasInit()) {
    return;
  }

  if (Result.Context->getSourceManager().isInSystemHeader(VD->getLocation())) {
    return;
  }

  diag(VD->getLocation(), "global variable '%0' detected") << VD->getName();
}

} // namespace codelint
} // namespace clang::tidy
