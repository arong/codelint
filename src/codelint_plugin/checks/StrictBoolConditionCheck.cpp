#include "codelint/checks/StrictBoolConditionCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>

namespace clang::tidy::codelint {

void StrictBoolConditionCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(ast_matchers::ifStmt().bind("ifStmt"), this);
  Finder->addMatcher(ast_matchers::whileStmt().bind("whileStmt"), this);
  Finder->addMatcher(ast_matchers::forStmt().bind("forStmt"), this);
  Finder->addMatcher(ast_matchers::doStmt().bind("doStmt"), this);
  Finder->addMatcher(ast_matchers::conditionalOperator().bind("condOp"), this);
}

void StrictBoolConditionCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const Expr* Cond{nullptr};

  if (const auto* IfStmtPtr = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt");
      IfStmtPtr != nullptr) {
    Cond = IfStmtPtr->getCond();
  } else if (const auto* WhileStmtPtr = Result.Nodes.getNodeAs<clang::WhileStmt>("whileStmt");
             WhileStmtPtr != nullptr) {
    Cond = WhileStmtPtr->getCond();
  } else if (const auto* ForStmtPtr = Result.Nodes.getNodeAs<clang::ForStmt>("forStmt");
             ForStmtPtr != nullptr) {
    Cond = ForStmtPtr->getCond();
  } else if (const auto* DoStmtPtr = Result.Nodes.getNodeAs<clang::DoStmt>("doStmt");
             DoStmtPtr != nullptr) {
    Cond = DoStmtPtr->getCond();
  } else if (const auto* CondOpPtr = Result.Nodes.getNodeAs<clang::ConditionalOperator>("condOp");
             CondOpPtr != nullptr) {
    Cond = CondOpPtr->getCond();
  }

  if (Cond != nullptr) {
    checkCondition(Cond, Result.Context);
  }
}

void StrictBoolConditionCheck::checkCondition(const clang::Expr* Cond, clang::ASTContext* Ctx) {
  if ((Cond == nullptr) || (Ctx == nullptr)) {
    return;
  }

  const auto& SrcMgr = Ctx->getSourceManager();
  if (const SourceLocation ExpansionLoc{SrcMgr.getExpansionLoc(Cond->getBeginLoc())};
      !SrcMgr.isInMainFile(ExpansionLoc)) {
    return;
  }

  if (isBoolType(Cond)) {
    return;
  }

  const clang::Expr* TrueCond{Cond->IgnoreImpCasts()};
  const clang::QualType CondType{TrueCond->getType()};

  diag(Cond->getBeginLoc(), "condition must be bool type, but got '%0'") << CondType.getAsString();
}

bool StrictBoolConditionCheck::isBoolType(const clang::Expr* expr) {
  if (expr == nullptr) {
    return false;
  }

  const clang::Expr* TrueExpr{expr->IgnoreImpCasts()};

  if (const clang::QualType qualType{TrueExpr->getType()}; qualType->isBooleanType()) {
    return true;
  }

  if (const auto* ICE = llvm::dyn_cast<clang::ImplicitCastExpr>(expr); ICE != nullptr) {
    if (const CastKind Kind{ICE->getCastKind()}; Kind == CK_IntegralToBoolean ||
                                                 Kind == CK_PointerToBoolean ||
                                                 Kind == CK_FloatingToBoolean) {
      return false;
    }
  }

  return false;
}

} // namespace clang::tidy::codelint
