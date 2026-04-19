#include "codelint/checks/SingletonCheck.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

namespace clang::tidy::codelint {

void SingletonCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  Finder->addMatcher(
      ast_matchers::functionDecl(
          ast_matchers::returns(ast_matchers::referenceType()),
          ast_matchers::has(ast_matchers::compoundStmt(
              ast_matchers::hasDescendant(
                  ast_matchers::varDecl(ast_matchers::isStaticStorageClass()).bind("staticLocal")),
              ast_matchers::hasDescendant(ast_matchers::returnStmt(
                  ast_matchers::has(ast_matchers::declRefExpr(ast_matchers::to(
                      ast_matchers::varDecl(ast_matchers::equalsBoundNode("staticLocal"))))))))))
          .bind("singletonFunc"),
      this);
}

void SingletonCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const auto* funcDecl = Result.Nodes.getNodeAs<clang::FunctionDecl>("singletonFunc");
  if (funcDecl == nullptr) {
    return;
  }

  auto& srcMgr = Result.Context->getSourceManager();
  const SourceLocation ExpansionLoc{srcMgr.getExpansionLoc(funcDecl->getLocation())};
  if (!srcMgr.isInMainFile(ExpansionLoc)) {
    return;
  }

  diag(funcDecl->getLocation(), "Meyer's Singleton pattern detected in '%0'")
      << funcDecl->getName();
}

} // namespace clang::tidy::codelint
