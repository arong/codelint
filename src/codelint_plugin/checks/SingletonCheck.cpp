#include "codelint/checks/SingletonCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace clang::tidy {
namespace codelint {

using namespace ast_matchers;

void SingletonCheck::registerMatchers(MatchFinder* Finder) {
  Finder->addMatcher(
      functionDecl(
          returns(referenceType()),
          has(compoundStmt(hasDescendant(varDecl(isStaticStorageClass()).bind("staticLocal")),
                           hasDescendant(returnStmt(
                               has(declRefExpr(to(varDecl(equalsBoundNode("staticLocal"))))))))))
          .bind("singletonFunc"),
      this);
}

void SingletonCheck::check(const MatchFinder::MatchResult& Result) {
  const auto* FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("singletonFunc");
  if (!FD) {
    return;
  }

  diag(FD->getLocation(), "Meyer's Singleton pattern detected in '%0'") << FD->getName();
}

} // namespace codelint
} // namespace clang::tidy
