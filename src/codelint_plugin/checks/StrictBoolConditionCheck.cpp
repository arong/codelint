#include "codelint/checks/StrictBoolConditionCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
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
  clang::QualType CondType{TrueCond->getType()};
  const clang::Expr* DiagExpr{TrueCond};

  if (const auto* UnaryOp = llvm::dyn_cast<clang::UnaryOperator>(TrueCond)) {
    if (UnaryOp->getOpcode() == clang::UnaryOperatorKind::UO_LNot) {
      const clang::Expr* Operand{UnaryOp->getSubExpr()->IgnoreImpCasts()};
      CondType = Operand->getType();
      DiagExpr = Operand;
    }
  } else if (const auto* BinOp = llvm::dyn_cast<clang::BinaryOperator>(TrueCond)) {
    if (BinOp->isLogicalOp()) {
      const clang::Expr* LHS{BinOp->getLHS()->IgnoreImpCasts()};
      const clang::Expr* RHS{BinOp->getRHS()->IgnoreImpCasts()};
      if (!isBoolType(BinOp->getLHS())) {
        CondType = LHS->getType();
        DiagExpr = LHS;
      } else if (!isBoolType(BinOp->getRHS())) {
        CondType = RHS->getType();
        DiagExpr = RHS;
      }
    }
  }

  diag(DiagExpr->getBeginLoc(), "condition must be bool type, but got '%0'")
      << CondType.getAsString();
}

bool StrictBoolConditionCheck::isBoolType(const clang::Expr* expr) {
  if (expr == nullptr) {
    return false;
  }

  const clang::Expr* TrueExpr{expr->IgnoreImpCasts()};

  // Logical NOT: validity depends on operand type (e.g., !pointer invalid, !bool OK)
  if (const auto* UnaryOp = llvm::dyn_cast<clang::UnaryOperator>(TrueExpr)) {
    if (UnaryOp->getOpcode() == clang::UnaryOperatorKind::UO_LNot) {
      return isBoolType(UnaryOp->getSubExpr());
    }
  }

  // Logical AND/OR: both operands must be bool
  if (const auto* BinOp = llvm::dyn_cast<clang::BinaryOperator>(TrueExpr)) {
    if (BinOp->isLogicalOp()) {
      return isBoolType(BinOp->getLHS()) && isBoolType(BinOp->getRHS());
    }
  }

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
