#include "codelint/checks/InitCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallVector.h>

namespace clang::tidy {
namespace codelint {

using clang::ast_matchers::autoType;
using clang::ast_matchers::cxxCatchStmt;
using clang::ast_matchers::cxxConstructorDecl;
using clang::ast_matchers::cxxForRangeStmt;
using clang::ast_matchers::expr;
using clang::ast_matchers::fieldDecl;
using clang::ast_matchers::forStmt;
using clang::ast_matchers::hasAncestor;
using clang::ast_matchers::hasCanonicalType;
using clang::ast_matchers::hasInitializer;
using clang::ast_matchers::hasParent;
using clang::ast_matchers::hasType;
using clang::ast_matchers::initListExpr;
using clang::ast_matchers::integerLiteral;
using clang::ast_matchers::isImplicit;
using clang::ast_matchers::isUnion;
using clang::ast_matchers::isUnsignedInteger;
using clang::ast_matchers::parmVarDecl;
using clang::ast_matchers::recordDecl;
using clang::ast_matchers::unless;
using clang::ast_matchers::varDecl;

void InitCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (!Finder) {
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
                         .bind("equals"),
                     this);

  Finder->addMatcher(
      varDecl(hasType(hasCanonicalType(isUnsignedInteger())), hasInitializer(integerLiteral()))
          .bind("unsigned"),
      this);

  Finder->addMatcher(varDecl(hasInitializer(initListExpr()), unless(parmVarDecl()),
                             unless(hasAncestor(forStmt())), unless(hasAncestor(cxxForRangeStmt())))
                         .bind("equals_brace"),
                     this);

  // Matcher for C++ constructors to check member initialization
  Finder->addMatcher(cxxConstructorDecl(unless(isImplicit())).bind("constructor"), this);
}

void InitCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (!Result.Context) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("uninit")) {
    checkUninitialized(VD, Result.Context);
  } else if (const auto* FD = Result.Nodes.getNodeAs<clang::FieldDecl>("uninit_field")) {
    checkUninitializedField(FD, Result.Context);
  } else if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("equals")) {
    checkEqualsInit(VD, Result.Context);
  } else if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("unsigned")) {
    checkUnsignedSuffix(VD, Result.Context);
  } else if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("equals_brace")) {
    checkEqualsBraceInit(VD, Result.Context);
  } else if (const auto* Ctor = Result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructor")) {
    checkUninitializedMemberVariablesInConstructors(Ctor, Result.Context);
  }
}

bool InitCheck::shouldSkipAuto(const VarDecl* VD) {
  if (!VD) {
    return false;
  }
  // Check if type contains auto (including auto*, const auto*, auto&, etc.)
  // getContainedAutoType() returns the AutoType if the type uses auto deduction,
  // even if the top-level type is a pointer or reference to auto.
  return VD->getType()->getContainedAutoType() != nullptr;
}

bool InitCheck::isAutoType(const VarDecl* VD) {
  if (!VD) {
    return false;
  }
  // Use TypeSourceInfo to detect auto in the source code
  // Even after deduction, getContainedAutoTypeLoc() returns the AutoTypeLoc
  // if the variable was declared with auto
  if (auto* TSI = VD->getTypeSourceInfo()) {
    AutoTypeLoc ATL = TSI->getTypeLoc().getContainedAutoTypeLoc();
    return !ATL.isNull();
  }
  return false;
}

bool InitCheck::shouldSkipUnion(const VarDecl* VD) {
  if (!VD) {
    return false;
  }
  const auto* DC = VD->getDeclContext();
  if (const auto* RD = dyn_cast<RecordDecl>(DC)) {
    return RD->isUnion();
  }
  return false;
}

bool InitCheck::shouldSkipExtern(const VarDecl* VD) {
  if (!VD) {
    return false;
  }
  return VD->getStorageClass() == SC_Extern;
}

bool InitCheck::hasExplicitInitializer(const VarDecl* VD) {
  if (!VD) {
    return false;
  }

  if (!VD->hasInit()) {
    return false;
  }

  auto InitStyle = VD->getInitStyle();
  if (InitStyle == VarDecl::CInit || InitStyle == VarDecl::ListInit) {
    return true;
  }

  const Expr* Init = VD->getInit();
  if (!Init) {
    return false;
  }

  const Expr* InitExpr = Init->IgnoreImplicit();

  if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr)) {
    if (CCE->isListInitialization()) {
      return true;
    }
    if (CCE->getNumArgs() > 0) {
      return true;
    }
    return false;
  }

  if (const auto* TOE = dyn_cast<CXXTemporaryObjectExpr>(InitExpr)) {
    return true;
  }

  return false;
}

bool InitCheck::shouldSkipEnumClass(const VarDecl* VD) {
  if (!VD) {
    return false;
  }
  if (!VD->getType()->isScopedEnumeralType()) {
    return false;
  }
  return !isEnumZeroValidType(VD->getType().getTypePtr());
}

bool InitCheck::isEnumZeroValidType(const Type* Ty) {
  if (!Ty || !Ty->isEnumeralType()) {
    return false;
  }

  const EnumDecl* Enum{nullptr};
  if (const auto* EnumTy = Ty->getAs<EnumType>()) {
    Enum = EnumTy->getDecl();
  } else if (const auto* ElabTy = Ty->getAs<ElaboratedType>()) {
    if (const auto* InnerEnumTy = ElabTy->getNamedType()->getAs<EnumType>()) {
      Enum = InnerEnumTy->getDecl();
    }
  }

  if (!Enum) {
    return false;
  }

  for (const auto* EnumConst : Enum->enumerators()) {
    if (EnumConst->getInitVal().isZero()) {
      return true;
    }
  }

  return false;
}

bool InitCheck::hasExplicitInitializer(const FieldDecl* FD) {
  if (!FD) {
    return false;
  }

  if (!FD->hasInClassInitializer()) {
    return false;
  }

  return true;
}

bool InitCheck::isInsideMacro(const VarDecl* VD, ASTContext* Ctx) {
  if (!VD || !Ctx) {
    return false;
  }

  auto& SM = Ctx->getSourceManager();
  SourceLocation Loc{VD->getLocation()};

  // Check if the declaration is inside a macro expansion
  return SM.isMacroBodyExpansion(Loc) || SM.isMacroArgExpansion(Loc);
}

bool InitCheck::hasInitializerListConstructor(const CXXRecordDecl* Record) {
  if (!Record) {
    return false;
  }

  for (const CXXConstructorDecl* Ctor : Record->ctors()) {
    if (Ctor->isExplicit()) {
      continue;
    }

    for (const ParmVarDecl* Param : Ctor->parameters()) {
      const Type* ParamTy = Param->getType().getTypePtr();
      if (const auto* TST = ParamTy->getAs<TemplateSpecializationType>()) {
        if (TST->getTemplateName().getAsTemplateDecl()->getName() == "initializer_list") {
          return true;
        }
      }
    }
  }

  return false;
}

bool InitCheck::hasNonTrivialDefaultConstructor(QualType QT) const {
  if (QT.isNull()) {
    return false;
  }

  if (QT->isArrayType()) {
    const Type* ElementType = QT->getArrayElementTypeNoTypeQual();
    return hasNonTrivialDefaultConstructor(QualType(ElementType, 0));
  }

  if (const auto* RD = QT->getAsCXXRecordDecl()) {
    if (RD->hasDefinition()) {
      return RD->hasNonTrivialDefaultConstructor();
    }
  }

  return false;
}

void InitCheck::checkUninitializedField(const FieldDecl* FD, ASTContext* Ctx) {
  if (!FD || !Ctx) {
    return;
  }

  const auto Name = FD->getName();
  if (Name.empty()) {
    return;
  }

  if (Ctx->getSourceManager().isInSystemHeader(FD->getLocation())) {
    return;
  }

  if (hasExplicitInitializer(FD)) {
    return;
  }

  if (FD->isBitField()) {
    return;
  }

  if (FD->getType()->isScopedEnumeralType() && !isEnumZeroValidType(FD->getType().getTypePtr())) {
    return;
  }

  if (FD->getType()->isReferenceType()) {
    return;
  }

  auto& SM = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto Loc = FD->getLocation();
  const auto EndLoc = Lexer::getLocForEndOfToken(FD->getLocation(), 0, SM, LangOpts);

  if (hasNonTrivialDefaultConstructor(FD->getType())) {
    diag(Loc, "field is not explicitly initialized") << FixItHint::CreateInsertion(EndLoc, "{}");
  } else {
    diag(Loc, "field is not initialized", DiagnosticIDs::Error)
        << FixItHint::CreateInsertion(EndLoc, "{}");
  }
}

void InitCheck::checkUninitialized(const VarDecl* VD, ASTContext* Ctx) {
  if (!VD || !Ctx) {
    return;
  }

  const auto Name = VD->getName();
  if (Name.empty()) {
    return;
  }

  // Skip checking if variable is inside a macro
  if (isInsideMacro(VD, Ctx)) {
    return;
  }

  if (shouldSkipAuto(VD) || shouldSkipUnion(VD) || shouldSkipExtern(VD) ||
      shouldSkipEnumClass(VD)) {
    return;
  }

  if (Ctx->getSourceManager().isInSystemHeader(VD->getLocation())) {
    return;
  }

  if (hasExplicitInitializer(VD)) {
    return;
  }

  // Check if this is a reference type - needs special handling since references must be initialized
  if (VD->getType()->isReferenceType()) {
    auto& SM = Ctx->getSourceManager();
    auto LangOpts = Ctx->getLangOpts();

    const auto Loc = VD->getLocation();

    diag(Loc, "reference variable is not initialized and must be bound to a value",
         DiagnosticIDs::Error);
    return;
  }

  auto& SM = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto Loc = VD->getLocation();
  SourceLocation EndLoc{};

  if (VD->getType()->isArrayType()) {
    if (auto* TSI = VD->getTypeSourceInfo()) {
      TypeLoc TL{TSI->getTypeLoc()};
      while (TL.getAs<ArrayTypeLoc>()) {
        ArrayTypeLoc ATL{TL.getAs<ArrayTypeLoc>()};
        TL = ATL.getElementLoc();
        EndLoc = ATL.getRBracketLoc().isValid() ? ATL.getRBracketLoc() : VD->getLocation();
      }
      if (EndLoc.isInvalid()) {
        EndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
      } else {
        EndLoc = Lexer::getLocForEndOfToken(EndLoc, 0, SM, LangOpts);
      }
    } else {
      EndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
    }
  } else {
    EndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
  }

  // Check if this is a C-style array and provide a more specific message
  if (VD->getType()->isArrayType()) {
    if (hasNonTrivialDefaultConstructor(VD->getType())) {
      diag(Loc, "C-style array should be initialized with braces '{}'")
          << FixItHint::CreateInsertion(EndLoc, "{}");
    } else {
      diag(Loc, "C-style array is not initialized", DiagnosticIDs::Error)
          << FixItHint::CreateInsertion(EndLoc, "{}");
    }
  } else {
    if (hasNonTrivialDefaultConstructor(VD->getType())) {
      diag(Loc, "variable is not explicitly initialized")
          << FixItHint::CreateInsertion(EndLoc, "{}");
    } else {
      diag(Loc, "variable is not initialized", DiagnosticIDs::Error)
          << FixItHint::CreateInsertion(EndLoc, "{}");
    }
  }
}

void InitCheck::checkEqualsInit(const VarDecl* VD, ASTContext* Ctx) {
  if (!VD || !Ctx) {
    return;
  }

  if (Ctx->getSourceManager().isInSystemHeader(VD->getLocation())) {
    return;
  }

  const auto Name = VD->getName();
  if (Name.empty()) {
    return;
  }

  auto InitStyle = VD->getInitStyle();
  if (InitStyle != VarDecl::CInit && InitStyle != VarDecl::CallInit) {
    return;
  }

  if (shouldSkipAuto(VD)) {
    return;
  }

  const auto* Init = VD->getInit();
  if (!Init) {
    return;
  }

  bool IsCallInit = (InitStyle == VarDecl::CallInit);
  if (IsCallInit) {
    const Expr* InitExpr = Init->IgnoreImplicit();
    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr)) {
      if (CCE->isListInitialization()) {
        return;
      }
    } else {
      return;
    }
  }

  if (InitStyle == VarDecl::CInit) {
    const Expr* InitExpr = Init->IgnoreImplicit();
    const Type* DestTy = VD->getType().getTypePtr();
    const Type* SrcTy = InitExpr->getType().getTypePtr();

    if (DestTy->isIntegerType() && SrcTy->isFloatingType()) {
      diag(VD->getLocation(),
           "narrowing conversion from floating to integer; cannot use '{}' initialization");
      return;
    }

    if (DestTy->isBooleanType() && SrcTy->isIntegerType() && !SrcTy->isBooleanType()) {
      diag(VD->getLocation(), "assigning integer to bool is dangerous; use explicit comparison",
           DiagnosticIDs::Error);
      return;
    }

    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr)) {
      if (CCE->isListInitialization()) {
        auto& SM = Ctx->getSourceManager();
        auto LangOpts = Ctx->getLangOpts();
        auto VarEndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
        VarEndLoc = Lexer::getLocForEndOfToken(VarEndLoc, 0, SM, LangOpts);
        auto InitStartLoc = Init->getBeginLoc();
        diag(VD->getLocation(), "initializer should use '{}' syntax instead of '= {}'")
            << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, InitStartLoc),
                                            "");
        return;
      }

      if (CCE->getConstructor()) {
        const CXXRecordDecl* Record{CCE->getConstructor()->getParent()};
        if (Record) {
          for (const CXXConstructorDecl* Ctor : Record->ctors()) {
            if (Ctor->isExplicit()) {
              continue;
            }
            for (const ParmVarDecl* Param : Ctor->parameters()) {
              const Type* ParamTy = Param->getType().getTypePtr();
              if (const auto* TST = ParamTy->getAs<TemplateSpecializationType>()) {
                if (TST->getTemplateName().getAsTemplateDecl()->getName() == "initializer_list") {
                  auto TemplateArgs = TST->template_arguments();
                  if (!TemplateArgs.empty()) {
                    const TemplateArgument& Arg = TemplateArgs[0];
                    QualType InitListElemType{Arg.getAsType()};
                    const Type* ArgTy{InitListElemType.getTypePtr()};

                    if (CCE->getNumArgs() > 0) {
                      const Expr* ArgExpr{CCE->getArg(0)};
                      const Type* ExprTy{ArgExpr->getType().getTypePtr()};
                      if (ExprTy->isIntegerType() && ArgTy->isIntegerType()) {
                        return;
                      }
                      if (ExprTy->isFloatingType() && ArgTy->isFloatingType()) {
                        return;
                      }
                      if (ExprTy->isPointerType() && ArgTy->isPointerType()) {
                        return;
                      }
                      if (ExprTy == ArgTy) {
                        return;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  auto& SM = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto InitRange = Init->getSourceRange();
  const auto Value = Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SM, LangOpts);

  auto VarEndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
  auto InitStartLoc = Init->getBeginLoc();

  const auto InitEnd = Lexer::getLocForEndOfToken(Init->getEndLoc(), 0, SM, LangOpts);

  std::string ClosingBrace = "}";
  const clang::Type* CanonicalTy =
      VD->getType().getTypePtr()->getCanonicalTypeInternal().getTypePtr();

  bool IsUnsignedInt = CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UInt) ||
                       CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UShort) ||
                       CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UChar) ||
                       CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char8) ||
                       CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char16) ||
                       CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char32);

  bool IsUnsignedLong = CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::ULong) ||
                        CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::ULongLong) ||
                        CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UInt128);

  const Expr* InitExpr = Init->IgnoreImplicit();
  if ((IsUnsignedInt || IsUnsignedLong) && isa<IntegerLiteral>(InitExpr)) {
    bool HasSuffix = false;
    for (char C : Value) {
      if (C == 'U' || C == 'u' || C == 'L' || C == 'l') {
        HasSuffix = true;
        break;
      }
    }
    if (!HasSuffix) {
      ClosingBrace = IsUnsignedLong ? "UL}" : "U}";
    }
  }

  if (IsCallInit) {
    const Expr* InitExpr = Init->IgnoreImplicit();
    const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr);
    if (CCE && CCE->getNumArgs() > 0) {
      const Expr* Arg = CCE->getArg(0);
      auto ArgStart = Arg->getBeginLoc();
      auto ParenCloseLoc = CCE->getParenOrBraceRange().getEnd();
      diag(VD->getLocation(), "variable should use '{}' syntax for initialization")
          << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, ArgStart), "{")
          << FixItHint::CreateReplacement(ParenCloseLoc, ClosingBrace);
    }
  } else {
    diag(VD->getLocation(), "variable should use '{}' syntax for initialization")
        << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, InitStartLoc), "{")
        << FixItHint::CreateInsertion(InitEnd, ClosingBrace);
  }
}

void InitCheck::checkUnsignedSuffix(const VarDecl* VD, ASTContext* Ctx) {
  if (!VD || !Ctx) {
    return;
  }

  const auto Name = VD->getName();
  if (Name.empty()) {
    return;
  }

  auto& SM = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto* Init = VD->getInit();
  if (!Init) {
    return;
  }

  const auto* IL = dyn_cast<IntegerLiteral>(Init);
  if (!IL) {
    return;
  }

  const auto InitRange = Init->getSourceRange();
  auto ValueText = Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SM, LangOpts);

  if (ValueText.empty() || ValueText.size() > 64) {
    return;
  }

  for (char C : ValueText) {
    if (C == 'U' || C == 'u' || C == 'L' || C == 'l') {
      return;
    }
  }

  const auto InitEnd = Lexer::getLocForEndOfToken(Init->getEndLoc(), 0, SM, LangOpts);

  QualType VarType = VD->getType();
  const clang::Type* CanonicalType = VarType.getTypePtr()->getCanonicalTypeInternal().getTypePtr();

  bool NeedsLongSuffix = false;
  if (CanonicalType->isSpecificBuiltinType(clang::BuiltinType::ULong) ||
      CanonicalType->isSpecificBuiltinType(clang::BuiltinType::ULongLong) ||
      CanonicalType->isSpecificBuiltinType(clang::BuiltinType::UInt128)) {
    NeedsLongSuffix = true;
  }

  std::string Suffix = NeedsLongSuffix ? "UL" : "U";

  diag(Init->getBeginLoc(), "unsigned integer literal should have '%0' suffix")
      << Suffix << FixItHint::CreateInsertion(InitEnd, Suffix);
}

void InitCheck::checkEqualsBraceInit(const VarDecl* VD, ASTContext* Ctx) {
  if (!VD || !Ctx) {
    return;
  }

  const auto Name = VD->getName();
  if (Name.empty()) {
    return;
  }

  const auto* Init = VD->getInit();
  if (!Init) {
    return;
  }

  const auto* ILE = dyn_cast<InitListExpr>(Init);
  if (!ILE) {
    return;
  }

  if (isInsideMacro(VD, Ctx)) {
    return;
  }

  auto& SM = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  auto VarEndLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);

  // For auto types, brace initialization should use '=' assignment instead
  // e.g., auto x{1} or auto x = {1} should become auto x = 1
  if (isAutoType(VD)) {
    if (ILE->getNumInits() == 1) {
      const Expr* SingleInit = ILE->getInit(0);
      auto BraceEndLoc = Lexer::getLocForEndOfToken(ILE->getRBraceLoc(), 0, SM, LangOpts);

      diag(VD->getLocation(), "auto type should use '=' assignment instead of brace initialization")
          << FixItHint::CreateReplacement(
                 CharSourceRange::getCharRange(VarEndLoc, BraceEndLoc),
                 " = " +
                     Lexer::getSourceText(
                         CharSourceRange::getTokenRange(SingleInit->getSourceRange()), SM, LangOpts)
                         .str());
    } else {
      diag(VD->getLocation(),
           "auto type should use '=' assignment instead of brace initialization");
    }
    return;
  }

  // For non-auto types with CInit style (= {}), suggest removing '=' to use direct brace init
  // e.g., int x = {1} should become int x{1}
  if (VD->getInitStyle() != VarDecl::CInit) {
    return;
  }

  SourceLocation BraceStartLoc;
  if (VD->getType()->isArrayType()) {
    if (auto* TSI = VD->getTypeSourceInfo()) {
      TypeLoc TL = TSI->getTypeLoc();
      while (TL.getAs<ArrayTypeLoc>()) {
        ArrayTypeLoc ATL = TL.getAs<ArrayTypeLoc>();
        TL = ATL.getElementLoc();
        BraceStartLoc = ATL.getRBracketLoc().isValid() ? ATL.getRBracketLoc() : VD->getLocation();
      }
      if (BraceStartLoc.isInvalid()) {
        BraceStartLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
      }
    } else {
      BraceStartLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
    }
  } else {
    BraceStartLoc = Lexer::getLocForEndOfToken(VD->getLocation(), 0, SM, LangOpts);
  }

  BraceStartLoc = Lexer::getLocForEndOfToken(BraceStartLoc, 0, SM, LangOpts);
  auto InitStartLoc = Init->getBeginLoc();

  diag(VD->getLocation(), "initializer should use '{}' syntax instead of '= {}'")
      << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, InitStartLoc), "");
}

void InitCheck::checkUninitializedMemberVariablesInConstructors(const CXXConstructorDecl* Ctor,
                                                                ASTContext* Ctx) {
  if (!Ctor || !Ctx || Ctor->isImplicit()) {
    return;
  }

  if (Ctx->getSourceManager().isInSystemHeader(Ctor->getLocation())) {
    return;
  }

  const CXXRecordDecl* Record = Ctor->getParent();
  if (!Record) {
    return;
  }

  // Collection to store member variables that are not initialized in this constructor
  llvm::SmallVector<const FieldDecl*, 16> UninitializedMembers;

  for (const FieldDecl* Field : Record->fields()) {
    // Skip members with in-class initializers (already handled by other checks)
    if (Field->hasInClassInitializer()) {
      continue;
    }

    // Skip bit fields
    if (Field->isBitField()) {
      continue;
    }

    // TODO: Need to determine how to properly identify static data members in LLVM 21+
    // For now, we'll skip this check, which means static members might get reported
    // (But typically static members are initialized elsewhere, not in constructors)

    // Check if this field is initialized in the constructor's initializer list
    bool InitializedInConstructor = false;
    for (const auto& Init : Ctor->inits()) {
      if (const FieldDecl* InitField = Init->getMember()) {
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

  // Report diagnostics for uninitialized members
  for (const FieldDecl* Field : UninitializedMembers) {
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

} // namespace codelint
} // namespace clang::tidy
