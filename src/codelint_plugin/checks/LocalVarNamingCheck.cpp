#include "codelint/checks/LocalVarNamingCheck.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

namespace clang::tidy::codelint {

void LocalVarNamingCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(ast_matchers::varDecl(ast_matchers::unless(ast_matchers::parmVarDecl()),
                                           ast_matchers::unless(ast_matchers::hasGlobalStorage()),
                                           ast_matchers::unless(ast_matchers::isStaticLocal()),
                                           ast_matchers::unless(ast_matchers::hasAncestor(
                                               ast_matchers::recordDecl())))
                         .bind("localVar"),
                     this);
}

void LocalVarNamingCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const auto* varDecl = Result.Nodes.getNodeAs<clang::VarDecl>("localVar");
  if (varDecl == nullptr || varDecl->isImplicit()) {
    return;
  }

  const auto& srcMgr = Result.Context->getSourceManager();
  if (const SourceLocation ExpansionLoc{srcMgr.getExpansionLoc(varDecl->getLocation())};
      !srcMgr.isInMainFile(ExpansionLoc)) {
    return;
  }

  StringRef name = varDecl->getName();
  if (name.empty()) {
    return;
  }

  if (name == "_") {
    return;
  }

  if (name.size() >= 2 && name[0] == 'm' && name[1] == '_') {
    diag(varDecl->getLocation(), "local variable '%0' should not use 'm_' prefix") << name;
  } else if (name[0] == '_') {
    diag(varDecl->getLocation(), "local variable '%0' should not start with '_'") << name;
  } else if (name[name.size() - 1] == '_') {
    diag(varDecl->getLocation(), "local variable '%0' should not end with '_'") << name;
  }
}

} // namespace clang::tidy::codelint
