#include "codelint/checks/StrictBoolConditionCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/Casting.h>

namespace clang::tidy {
namespace codelint {

void StrictBoolConditionCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (!Finder) {
    return;
  }

  Finder->addMatcher(ast_matchers::ifStmt().bind("ifStmt"), this);
  Finder->addMatcher(ast_matchers::whileStmt().bind("whileStmt"), this);
  Finder->addMatcher(ast_matchers::forStmt().bind("forStmt"), this);
  Finder->addMatcher(ast_matchers::doStmt().bind("doStmt"), this);
  Finder->addMatcher(ast_matchers::conditionalOperator().bind("condOp"), this);
}

void StrictBoolConditionCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (!Result.Context) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const clang::Expr* Cond{nullptr};

  if (const auto* IS = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt")) {
    Cond = IS->getCond();
  } else if (const auto* WS = Result.Nodes.getNodeAs<clang::WhileStmt>("whileStmt")) {
    Cond = WS->getCond();
  } else if (const auto* FS = Result.Nodes.getNodeAs<clang::ForStmt>("forStmt")) {
    Cond = FS->getCond();
  } else if (const auto* DS = Result.Nodes.getNodeAs<clang::DoStmt>("doStmt")) {
    Cond = DS->getCond();
  } else if (const auto* CO = Result.Nodes.getNodeAs<clang::ConditionalOperator>("condOp")) {
    Cond = CO->getCond();
  }

  if (Cond) {
    checkCondition(Cond, Result.Context);
  }
}

void StrictBoolConditionCheck::checkCondition(const clang::Expr* Cond, clang::ASTContext* Ctx) {
  if (!Cond || !Ctx) {
    return;
  }

  if (isBoolType(Cond)) {
    return;
  }

  const clang::Expr* TrueCond{Cond->IgnoreImpCasts()};
  clang::QualType CondType{TrueCond->getType()};

  diag(Cond->getBeginLoc(), "condition must be bool type, but got '%0'") << CondType.getAsString();
}

bool StrictBoolConditionCheck::isBoolType(const clang::Expr* E) {
  if (!E) {
    return false;
  }

  const clang::Expr* TrueExpr{E->IgnoreImpCasts()};
  clang::QualType Ty{TrueExpr->getType()};

  if (Ty->isBooleanType()) {
    return true;
  }

  if (const auto* ICE = llvm::dyn_cast<clang::ImplicitCastExpr>(E)) {
    clang::CastKind Kind{ICE->getCastKind()};
    if (Kind == clang::CK_IntegralToBoolean || Kind == clang::CK_PointerToBoolean ||
        Kind == clang::CK_FloatingToBoolean) {
      return false;
    }
  }

  return false;
}

} // namespace codelint
} // namespace clang::tidy
