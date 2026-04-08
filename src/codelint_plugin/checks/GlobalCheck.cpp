#include "codelint/checks/GlobalCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

namespace clang::tidy {
namespace codelint {

using namespace clang::ast_matchers;

void GlobalCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (!Finder) {
    return;
  }

  Finder->addMatcher(varDecl(hasGlobalStorage(), unless(isStaticLocal()), unless(parmVarDecl()),
                             unless(hasAncestor(recordDecl())))
                         .bind("globalVar"),
                     this);
}

void GlobalCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("globalVar");
  if (!VD || VD->isImplicit() || VD->getName().empty()) {
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
