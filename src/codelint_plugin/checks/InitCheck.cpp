#include "codelint/checks/InitCheck.h"
#include "codelint/utils/InitUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallVector.h>

namespace clang::tidy::codelint {

using namespace ast_matchers;
using utils::hasExplicitInitializer;
using utils::hasNonTrivialDefaultConstructor;
using utils::isInsideMacro;
using utils::isInSystemHeader;
using utils::shouldSkipAuto;
using utils::shouldSkipEnumClass;
using utils::shouldSkipExtern;
using utils::shouldSkipUnion;

void InitCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(varDecl(unless(parmVarDecl()), unless(hasType(autoType())),
                             unless(hasAncestor(cxxForRangeStmt())), unless(hasAncestor(forStmt())),
                             unless(hasAncestor(recordDecl(isUnion()))),
                             unless(hasParent(cxxCatchStmt())))
                         .bind("uninit"),
                     this);

  Finder->addMatcher(fieldDecl(unless(hasAncestor(recordDecl(isUnion())))).bind("uninit_field"),
                     this);

  Finder->addMatcher(varDecl(hasInitializer(expr()), unless(hasInitializer(initListExpr())),
                             unless(hasType(autoType())), unless(parmVarDecl()),
                             unless(hasParent(cxxCatchStmt())), unless(hasAncestor(forStmt())),
                             unless(hasAncestor(cxxForRangeStmt())))
                         .bind("dangerous"),
                     this);

  Finder->addMatcher(cxxConstructorDecl(unless(isImplicit())).bind("constructor"), this);
}

void InitCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("uninit"); VarDeclPtr != nullptr) {
    checkUninitialized(VarDeclPtr, Result.Context);
  } else if (const auto* FieldDeclPtr = Result.Nodes.getNodeAs<FieldDecl>("uninit_field");
             FieldDeclPtr != nullptr) {
    checkUninitializedField(FieldDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("dangerous");
             VarDeclPtr != nullptr) {
    checkDangerousConversion(VarDeclPtr, Result.Context);
  } else if (const auto* Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("constructor");
             Ctor != nullptr) {
    checkUninitializedMemberVariablesInConstructors(Ctor, Result.Context);
  }
}

void InitCheck::checkUninitializedField(const FieldDecl* FieldDeclPtr, ASTContext* Ctx) {
  if ((FieldDeclPtr == nullptr) || (Ctx == nullptr)) {
    return;
  }

  const auto Name = FieldDeclPtr->getName();
  if (Name.empty()) {
    return;
  }

  if (isInSystemHeader(FieldDeclPtr->getLocation(), Ctx)) {
    return;
  }

  if (hasExplicitInitializer(FieldDeclPtr)) {
    return;
  }

  if (FieldDeclPtr->isBitField()) {
    return;
  }

  if (FieldDeclPtr->getType()->isScopedEnumeralType() &&
      !utils::isEnumZeroValidType(FieldDeclPtr->getType().getTypePtr())) {
    return;
  }

  if (FieldDeclPtr->getType()->isReferenceType()) {
    return;
  }

  auto& SrcMgr = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto Loc = FieldDeclPtr->getLocation();
  const auto EndLoc = Lexer::getLocForEndOfToken(FieldDeclPtr->getLocation(), 0, SrcMgr, LangOpts);

  if (hasNonTrivialDefaultConstructor(FieldDeclPtr->getType())) {
    diag(Loc, "field is not explicitly initialized") << FixItHint::CreateInsertion(EndLoc, "{}");
  } else {
    diag(Loc, "field is not initialized", DiagnosticIDs::Error)
        << FixItHint::CreateInsertion(EndLoc, "{}");
  }
}

void InitCheck::checkUninitialized(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
  if ((VarDeclPtr == nullptr) || (Ctx == nullptr)) {
    return;
  }

  const auto Name = VarDeclPtr->getName();
  if (Name.empty()) {
    return;
  }

  if (isInsideMacro(VarDeclPtr, Ctx)) {
    return;
  }

  if (shouldSkipAuto(VarDeclPtr) || shouldSkipUnion(VarDeclPtr) || shouldSkipExtern(VarDeclPtr) ||
      shouldSkipEnumClass(VarDeclPtr)) {
    return;
  }

  if (VarDeclPtr->isStaticDataMember()) {
    return;
  }

  if (isInSystemHeader(VarDeclPtr->getLocation(), Ctx)) {
    return;
  }

  if (hasExplicitInitializer(VarDeclPtr)) {
    return;
  }

  if (VarDeclPtr->getType()->isReferenceType()) {
    auto LangOpts = Ctx->getLangOpts();
    const auto Loc = VarDeclPtr->getLocation();

    diag(Loc, "reference variable is not initialized and must be bound to a value",
         DiagnosticIDs::Error);
    return;
  }

  const auto& SrcMgr = Ctx->getSourceManager();
  const auto LangOpts = Ctx->getLangOpts();

  const auto Loc = VarDeclPtr->getLocation();
  SourceLocation EndLoc{};

  if (VarDeclPtr->getType()->isArrayType()) {
    if (auto* TSI = VarDeclPtr->getTypeSourceInfo(); TSI != nullptr) {
      TypeLoc TypeLocRef{TSI->getTypeLoc()};
      while (TypeLocRef.getAs<ArrayTypeLoc>()) {
        const ArrayTypeLoc ArrayTypeLocRef{TypeLocRef.getAs<ArrayTypeLoc>()};
        TypeLocRef = ArrayTypeLocRef.getElementLoc();
        EndLoc = ArrayTypeLocRef.getRBracketLoc().isValid() ? ArrayTypeLocRef.getRBracketLoc()
                                                            : VarDeclPtr->getLocation();
      }
      if (EndLoc.isInvalid()) {
        EndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
      } else {
        EndLoc = Lexer::getLocForEndOfToken(EndLoc, 0, SrcMgr, LangOpts);
      }
    } else {
      EndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
    }
  } else {
    EndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
  }

  if (VarDeclPtr->getType()->isArrayType()) {
    if (hasNonTrivialDefaultConstructor(VarDeclPtr->getType())) {
      diag(Loc, "C-style array should be initialized with braces '{}'")
          << FixItHint::CreateInsertion(EndLoc, "{}");
    } else {
      diag(Loc, "C-style array is not initialized", DiagnosticIDs::Error)
          << FixItHint::CreateInsertion(EndLoc, "{}");
    }
  } else {
    if (hasNonTrivialDefaultConstructor(VarDeclPtr->getType())) {
      diag(Loc, "variable is not explicitly initialized")
          << FixItHint::CreateInsertion(EndLoc, "{}");
    } else {
      diag(Loc, "variable is not initialized", DiagnosticIDs::Error)
          << FixItHint::CreateInsertion(EndLoc, "{}");
    }
  }
}

void InitCheck::checkDangerousConversion(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
  if ((VarDeclPtr == nullptr) || (Ctx == nullptr)) {
    return;
  }

  if (isInSystemHeader(VarDeclPtr->getLocation(), Ctx)) {
    return;
  }

  const auto Name = VarDeclPtr->getName();
  if (Name.empty()) {
    return;
  }

  auto InitStyle = VarDeclPtr->getInitStyle();
  if (InitStyle != VarDecl::CInit) {
    return;
  }

  if (shouldSkipAuto(VarDeclPtr)) {
    return;
  }

  if (isInsideMacro(VarDeclPtr, Ctx)) {
    return;
  }

  const auto* Init = VarDeclPtr->getInit();
  if (Init == nullptr) {
    return;
  }

  const Expr* InitExpr{Init->IgnoreImplicit()};
  const Type* DestTy{VarDeclPtr->getType().getTypePtr()};
  const Type* SrcTy{InitExpr->getType().getTypePtr()};

  if (DestTy->isIntegerType() && SrcTy->isFloatingType()) {
    diag(VarDeclPtr->getLocation(),
         "narrowing conversion from floating to integer; cannot use '{}' initialization");
    return;
  }

  if (DestTy->isBooleanType() && SrcTy->isIntegerType() && !SrcTy->isBooleanType()) {
    diag(VarDeclPtr->getLocation(),
         "assigning integer to bool is dangerous; use explicit comparison", DiagnosticIDs::Error);
    return;
  }
}

void InitCheck::checkUninitializedMemberVariablesInConstructors(const CXXConstructorDecl* Ctor,
                                                                ASTContext* Ctx) {
  if ((Ctor == nullptr) || (Ctx == nullptr) || Ctor->isImplicit()) {
    return;
  }

  if (isInSystemHeader(Ctor->getLocation(), Ctx)) {
    return;
  }

  const CXXRecordDecl* Record{Ctor->getParent()};
  if (Record == nullptr) {
    return;
  }

  constexpr std::size_t kInitialMemberCapacity = 16;
  llvm::SmallVector<const FieldDecl*, kInitialMemberCapacity> UninitializedMembers{};

  for (const FieldDecl* Field : Record->fields()) {
    if (Field->hasInClassInitializer()) {
      continue;
    }

    if (Field->isBitField()) {
      continue;
    }

    bool InitializedInConstructor = false;
    for (const auto& Init : Ctor->inits()) {
      if (const FieldDecl* InitField = Init->getMember(); InitField != nullptr) {
        if (InitField == Field) {
          InitializedInConstructor = true;
          break;
        }
      }
    }

    if (!InitializedInConstructor) {
      UninitializedMembers.push_back(Field);
    }
  }

  for (const FieldDecl* Field : UninitializedMembers) {
    if (isInSystemHeader(Field->getLocation(), Ctx)) {
      continue;
    }

    const auto Loc = Field->getLocation();
    if (hasNonTrivialDefaultConstructor(Field->getType())) {
      diag(Loc, "member variable '%0' is not explicitly initialized in constructor")
          << Field->getName();
    } else {
      diag(Loc, "member variable '%0' is not initialized in constructor", DiagnosticIDs::Error)
          << Field->getName();
    }
  }
}

} // namespace clang::tidy::codelint
