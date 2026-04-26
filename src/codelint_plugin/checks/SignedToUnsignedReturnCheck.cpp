#include "codelint/checks/SignedToUnsignedReturnCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

namespace clang::tidy::codelint {

void SignedToUnsignedReturnCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(
      ast_matchers::varDecl(ast_matchers::hasInitializer(ast_matchers::ignoringImpCasts(
                                ast_matchers::callExpr().bind("callExpr"))))
          .bind("varDecl"),
      this);

  Finder->addMatcher(
      ast_matchers::binaryOperator(ast_matchers::hasOperatorName("="),
                                   ast_matchers::hasLHS(ast_matchers::expr().bind("lhsExpr")),
                                   ast_matchers::hasRHS(ast_matchers::ignoringImpCasts(
                                       ast_matchers::callExpr().bind("callExpr2"))))
          .bind("assignExpr"),
      this);
}

void SignedToUnsignedReturnCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  ASTContext* Ctx = Result.Context;
  const SourceManager& SrcMgr = Ctx->getSourceManager();

  if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("varDecl")) {
    if (const auto* CallExprPtr = Result.Nodes.getNodeAs<CallExpr>("callExpr")) {
      checkSignedToUnsignedConversion(VarDeclPtr, CallExprPtr, Ctx, SrcMgr);
    }
  }

  if (const auto* AssignExprPtr = Result.Nodes.getNodeAs<BinaryOperator>("assignExpr")) {
    if (const auto* LHSExprPtr = Result.Nodes.getNodeAs<Expr>("lhsExpr")) {
      if (const auto* CallExprPtr = Result.Nodes.getNodeAs<CallExpr>("callExpr2")) {
        checkSignedToUnsignedAssignment(LHSExprPtr, CallExprPtr, Ctx, SrcMgr);
      }
    }
  }
}

void SignedToUnsignedReturnCheck::checkSignedToUnsignedConversion(const VarDecl* VarDeclPtr,
                                                                  const CallExpr* CallExprPtr,
                                                                  ASTContext* Ctx,
                                                                  const SourceManager& SrcMgr) {
  if (VarDeclPtr == nullptr || CallExprPtr == nullptr) {
    return;
  }

  SourceLocation ExpansionLoc = SrcMgr.getExpansionLoc(VarDeclPtr->getBeginLoc());
  if (!SrcMgr.isInMainFile(ExpansionLoc)) {
    return;
  }

  QualType VarType = VarDeclPtr->getType();
  QualType ReturnType = CallExprPtr->getType();

  if (!isUnsignedType(VarType)) {
    return;
  }

  if (!isSignedType(ReturnType)) {
    return;
  }

  std::string FuncName = getFunctionName(CallExprPtr);

  diag(VarDeclPtr->getBeginLoc(),
       "signed return value from '%0' (type '%1') is assigned to unsigned variable "
       "'%2' (type '%3'); this may cause errors when function returns negative values")
      << FuncName << ReturnType.getAsString() << VarDeclPtr->getNameAsString()
      << VarType.getAsString();
}

void SignedToUnsignedReturnCheck::checkSignedToUnsignedAssignment(const Expr* LHSExprPtr,
                                                                  const CallExpr* CallExprPtr,
                                                                  ASTContext* Ctx,
                                                                  const SourceManager& SrcMgr) {
  if (LHSExprPtr == nullptr || CallExprPtr == nullptr) {
    return;
  }

  SourceLocation ExpansionLoc = SrcMgr.getExpansionLoc(LHSExprPtr->getBeginLoc());
  if (!SrcMgr.isInMainFile(ExpansionLoc)) {
    return;
  }

  const Expr* TrueLHS = LHSExprPtr->IgnoreImpCasts();
  QualType LHSType = TrueLHS->getType();

  if (!isUnsignedType(LHSType)) {
    return;
  }

  QualType ReturnType = CallExprPtr->getType();

  if (!isSignedType(ReturnType)) {
    return;
  }

  std::string FuncName = getFunctionName(CallExprPtr);

  diag(LHSExprPtr->getBeginLoc(),
       "signed return value from '%0' (type '%1') is assigned to unsigned expression "
       "(type '%2'); this may cause errors when function returns negative values")
      << FuncName << ReturnType.getAsString() << LHSType.getAsString();
}

bool SignedToUnsignedReturnCheck::isUnsignedType(const QualType& QT) {
  if (QT.isNull()) {
    return false;
  }

  const clang::Type* InnerType = QT.getTypePtr();

  if (InnerType->isUnsignedIntegerType()) {
    return true;
  }

  if (const auto* TDT = llvm::dyn_cast<clang::TypedefType>(InnerType)) {
    return TDT->desugar()->isUnsignedIntegerType();
  }

  if (const auto* ET = llvm::dyn_cast<clang::ElaboratedType>(InnerType)) {
    return isUnsignedType(ET->getNamedType());
  }

  return false;
}

bool SignedToUnsignedReturnCheck::isSignedType(const QualType& QT) {
  if (QT.isNull()) {
    return false;
  }

  const clang::Type* InnerType = QT.getTypePtr();

  if (InnerType->isSignedIntegerType()) {
    return true;
  }

  if (const auto* TDT = llvm::dyn_cast<clang::TypedefType>(InnerType)) {
    return TDT->desugar()->isSignedIntegerType();
  }

  if (const auto* ET = llvm::dyn_cast<clang::ElaboratedType>(InnerType)) {
    return isSignedType(ET->getNamedType());
  }

  return false;
}

std::string SignedToUnsignedReturnCheck::getFunctionName(const CallExpr* Call) {
  if (Call == nullptr) {
    return "";
  }

  const FunctionDecl* FuncDecl = Call->getDirectCallee();
  if (FuncDecl == nullptr) {
    return "";
  }

  return FuncDecl->getNameInfo().getName().getAsString();
}

} // namespace clang::tidy::codelint
