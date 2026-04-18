#include "codelint/checks/StrictBoolConditionCheck.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"

using namespace clang::ast_matchers;

namespace clang::tidy {
namespace codelint {

void StrictBoolConditionCheck::registerMatchers(MatchFinder* Finder) {
  if (!Finder)
    return;

  Finder->addMatcher(ifStmt().bind("ifStmt"), this);
  Finder->addMatcher(whileStmt().bind("whileStmt"), this);
  Finder->addMatcher(forStmt().bind("forStmt"), this);
  Finder->addMatcher(doStmt().bind("doStmt"), this);
  Finder->addMatcher(conditionalOperator().bind("condOp"), this);
}

void StrictBoolConditionCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (!Result.Context)
    return;

  if (Result.Context->getDiagnostics().hasErrorOccurred())
    return;

  const Expr* Cond = nullptr;

  if (const auto* IS = Result.Nodes.getNodeAs<IfStmt>("ifStmt")) {
    Cond = IS->getCond();
  } else if (const auto* WS = Result.Nodes.getNodeAs<WhileStmt>("whileStmt")) {
    Cond = WS->getCond();
  } else if (const auto* FS = Result.Nodes.getNodeAs<ForStmt>("forStmt")) {
    Cond = FS->getCond();
  } else if (const auto* DS = Result.Nodes.getNodeAs<DoStmt>("doStmt")) {
    Cond = DS->getCond();
  } else if (const auto* CO = Result.Nodes.getNodeAs<ConditionalOperator>("condOp")) {
    Cond = CO->getCond();
  }

  if (Cond) {
    checkCondition(Cond, Result.Context);
  }
}

void StrictBoolConditionCheck::checkCondition(const Expr* Cond, ASTContext* Ctx) {
  if (!Cond || !Ctx)
    return;

  if (isBoolType(Cond))
    return;

  const Expr* TrueCond = Cond->IgnoreImpCasts();
  QualType CondType = TrueCond->getType();

  diag(Cond->getBeginLoc(), "condition must be bool type, but got '%0'") << CondType.getAsString();
}

bool StrictBoolConditionCheck::isBoolType(const Expr* E) {
  if (!E)
    return false;

  const Expr* TrueExpr = E->IgnoreImpCasts();
  QualType Ty = TrueExpr->getType();

  if (Ty->isBooleanType())
    return true;

  if (const auto* ICE = dyn_cast<ImplicitCastExpr>(E)) {
    CastKind Kind = ICE->getCastKind();
    if (Kind == CK_IntegralToBoolean || Kind == CK_PointerToBoolean ||
        Kind == CK_FloatingToBoolean) {
      return false;
    }
  }

  return false;
}

} // namespace codelint
} // namespace clang::tidy
