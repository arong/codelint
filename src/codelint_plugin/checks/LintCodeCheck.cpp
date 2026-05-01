#include "codelint/checks/LintCodeCheck.h"
#include "codelint/utils/InitUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace clang::tidy::codelint {

constexpr std::size_t kMaxValueTextLength = 64;

using namespace ast_matchers;
using utils::isAutoType;
using utils::isInsideMacro;
using utils::isInSystemHeader;
using utils::shouldSkipAuto;

void LintCodeCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

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
}

void LintCodeCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("equals"); VarDeclPtr != nullptr) {
    checkEqualsInit(VarDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("unsigned");
             VarDeclPtr != nullptr) {
    checkUnsignedSuffix(VarDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<VarDecl>("equals_brace");
             VarDeclPtr != nullptr) {
    checkEqualsBraceInit(VarDeclPtr, Result.Context);
  }
}

std::optional<bool>
LintCodeCheck::wouldBraceInitChangeBasicStringConstructor(const CXXConstructExpr* CCE,
                                                          const CXXRecordDecl* Record) {
  if (CCE == nullptr || Record == nullptr) {
    return std::nullopt;
  }

  const std::string RecordName = Record->getQualifiedNameAsString();
  if (RecordName != "std::basic_string") {
    return std::nullopt;
  }

  const unsigned NumArgs = CCE->getNumArgs();

  auto isCharOrWCharType = [](const Type* Ty) {
    if (Ty == nullptr) {
      return false;
    }
    if (Ty->isSpecificBuiltinType(BuiltinType::Char_U) ||
        Ty->isSpecificBuiltinType(BuiltinType::Char_S) ||
        Ty->isSpecificBuiltinType(BuiltinType::UChar) ||
        Ty->isSpecificBuiltinType(BuiltinType::SChar) ||
        Ty->isSpecificBuiltinType(BuiltinType::Char8)) {
      return true;
    }
    if (Ty->isSpecificBuiltinType(BuiltinType::WChar_U) ||
        Ty->isSpecificBuiltinType(BuiltinType::WChar_S)) {
      return true;
    }
    return false;
  };

  auto isCharPointerType = [&isCharOrWCharType](const Type* Ty) {
    if (Ty == nullptr || !Ty->isPointerType()) {
      return false;
    }
    const Type* PointeeTy = Ty->getPointeeType().getTypePtr();
    return isCharOrWCharType(PointeeTy);
  };

  auto isAllocatorType = [](const Type* Ty) {
    if (Ty == nullptr) {
      return false;
    }
    std::string TypeName = Ty->getCanonicalTypeInternal().getAsString();
    return TypeName.find("allocator") != std::string::npos;
  };

  if (NumArgs == 2) {
    const Expr* Arg0 = CCE->getArg(0);
    const Expr* Arg1 = CCE->getArg(1);
    const Type* Arg0Ty = Arg0->getType().getTypePtr();
    const Type* Arg1Ty = Arg1->getType().getTypePtr();

    if (isCharPointerType(Arg0Ty) && isAllocatorType(Arg1Ty)) {
      return false;
    }

    if (isCharPointerType(Arg0Ty) && isCharPointerType(Arg1Ty)) {
      return false;
    }
  }

  if (NumArgs == 1) {
    const Expr* ArgExpr = CCE->getArg(0);
    const Type* ArgTy = ArgExpr->getType().getTypePtr();

    if (ArgTy->isPointerType()) {
      const Type* PointeeTy = ArgTy->getPointeeType().getTypePtr();
      if (isCharOrWCharType(PointeeTy)) {
        return false;
      }
    }

    if (const ArrayType* ArrTy = ArgTy->getAsArrayTypeUnsafe(); ArrTy != nullptr) {
      const Type* ElemTy = ArrTy->getElementType().getTypePtr();
      if (isCharOrWCharType(ElemTy)) {
        return false;
      }
    }

    const Expr* ArgWithoutImplicit = ArgExpr->IgnoreImplicit();
    if (isa<StringLiteral>(ArgWithoutImplicit)) {
      return false;
    }
    const Type* ArgWithoutImplicitTy = ArgWithoutImplicit->getType().getTypePtr();
    if (const ArrayType* ArrTy = ArgWithoutImplicitTy->getAsArrayTypeUnsafe(); ArrTy != nullptr) {
      const Type* ElemTy = ArrTy->getElementType().getTypePtr();
      if (isCharOrWCharType(ElemTy)) {
        return false;
      }
    }
  }

  return std::nullopt;
}

bool LintCodeCheck::wouldBraceInitChangeConstructor(const CXXConstructExpr* CCE) {
  if (CCE == nullptr) {
    return false;
  }

  const CXXConstructorDecl* CurrentCtor = CCE->getConstructor();
  if (CurrentCtor == nullptr) {
    return false;
  }

  const CXXRecordDecl* Record = CurrentCtor->getParent();
  if (Record == nullptr) {
    return false;
  }

  bool HasInitializerListCtor = false;
  QualType InitListElemType;

  for (const CXXConstructorDecl* Ctor : Record->ctors()) {
    if (Ctor->isExplicit()) {
      continue;
    }

    for (const ParmVarDecl* Param : Ctor->parameters()) {
      const Type* ParamTy = Param->getType().getTypePtr();
      if (const auto* TST = ParamTy->getAs<TemplateSpecializationType>(); TST != nullptr) {
        const auto* TemplateDecl = TST->getTemplateName().getAsTemplateDecl();
        if (TemplateDecl == nullptr) {
          continue;
        }

        const auto TemplateName = TemplateDecl->getName();
        bool IsInitList = (TemplateName == "initializer_list");

        if (IsInitList) {
          HasInitializerListCtor = true;
          auto TemplateArgs = TST->template_arguments();
          if (!TemplateArgs.empty()) {
            InitListElemType = TemplateArgs[0].getAsType();
          }
          break;
        }
      }
    }
    if (HasInitializerListCtor) {
      break;
    }
  }

  if (!HasInitializerListCtor) {
    return false;
  }

  const unsigned NumArgs = CCE->getNumArgs();

  if (NumArgs >= 2) {
    auto BasicStringResult = wouldBraceInitChangeBasicStringConstructor(CCE, Record);
    if (BasicStringResult.has_value()) {
      return BasicStringResult.value();
    }

    if (NumArgs == 2 && CurrentCtor->getNumParams() >= 2) {
      const QualType Param1Ty = CurrentCtor->getParamDecl(0)->getType();
      const QualType Param2Ty = CurrentCtor->getParamDecl(1)->getType();

      bool Param1IsIterator =
          Param1Ty->getAs<PointerType>() != nullptr || Param1Ty->getAs<ReferenceType>() != nullptr;
      bool Param2IsIterator =
          Param2Ty->getAs<PointerType>() != nullptr || Param2Ty->getAs<ReferenceType>() != nullptr;

      if (Param1IsIterator && Param2IsIterator && CCE->getArg(0)->getType()->isPointerType() &&
          CCE->getArg(1)->getType()->isPointerType()) {
        return false;
      }
    }

    return true;
  }

  if (NumArgs == 1) {
    auto BasicStringResult = wouldBraceInitChangeBasicStringConstructor(CCE, Record);
    if (BasicStringResult.has_value()) {
      return BasicStringResult.value();
    }

    const Expr* ArgExpr = CCE->getArg(0);
    const Type* ArgTy = ArgExpr->getType().getTypePtr();

    if (!InitListElemType.isNull()) {
      const Type* ElemTy = InitListElemType.getTypePtr();

      if (ArgTy->isPointerType() && !ElemTy->isPointerType()) {
        return false;
      }

      if (ArgTy->isIntegerType() && ElemTy->isIntegerType()) {
        return true;
      }

      if (ArgTy->isFloatingType() && ElemTy->isFloatingType()) {
        return true;
      }
    }

    return false;
  }

  return false;
}

void LintCodeCheck::checkEqualsInit(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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
  if (InitStyle != VarDecl::CInit && InitStyle != VarDecl::CallInit) {
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

  const bool IsCallInit{(InitStyle == VarDecl::CallInit)};
  if (IsCallInit) {
    const Expr* InitExpr{Init->IgnoreImplicit()};
    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr); CCE != nullptr) {
      if (CCE->isListInitialization()) {
        return;
      }
    } else {
      return;
    }
  }

  const Expr* InitExpr{Init->IgnoreImplicit()};
  const Type* DestTy{VarDeclPtr->getType().getTypePtr()};
  const Type* SrcTy{InitExpr->getType().getTypePtr()};

  if (DestTy->isIntegerType() && SrcTy->isFloatingType()) {
    return;
  }

  if (DestTy->isBooleanType() && SrcTy->isIntegerType() && !SrcTy->isBooleanType()) {
    return;
  }

  if (InitStyle == VarDecl::CInit) {
    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr); CCE != nullptr) {
      if (CCE->isListInitialization()) {
        auto& SrcMgr = Ctx->getSourceManager();
        auto LangOpts = Ctx->getLangOpts();
        auto VarEndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
        VarEndLoc = Lexer::getLocForEndOfToken(VarEndLoc, 0, SrcMgr, LangOpts);
        auto InitStartLoc = Init->getBeginLoc();
        diag(VarDeclPtr->getLocation(), "initializer should use '{}' syntax instead of '= {}'")
            << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, InitStartLoc),
                                            "");
        return;
      }

      if (wouldBraceInitChangeConstructor(CCE)) {
        return;
      }
    }
  }

  auto& SrcMgr = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto InitRange = Init->getSourceRange();
  const auto Value =
      Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SrcMgr, LangOpts);

  auto VarEndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
  auto InitStartLoc = Init->getBeginLoc();

  const auto InitEnd = Lexer::getLocForEndOfToken(Init->getEndLoc(), 0, SrcMgr, LangOpts);

  std::string ClosingBrace{"}"};
  const clang::Type* CanonicalTy{
      VarDeclPtr->getType().getTypePtr()->getCanonicalTypeInternal().getTypePtr()};

  const bool IsUnsignedInt{CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UInt) ||
                           CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UShort) ||
                           CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UChar) ||
                           CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char8) ||
                           CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char16) ||
                           CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::Char32)};

  const bool IsUnsignedLong{CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::ULong) ||
                            CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::ULongLong) ||
                            CanonicalTy->isSpecificBuiltinType(clang::BuiltinType::UInt128)};

  if (const Expr* InitExprInner{Init->IgnoreImplicit()};
      (IsUnsignedInt || IsUnsignedLong) && isa<IntegerLiteral>(InitExprInner)) {
    bool HasSuffix{false};
    for (const char Ch : Value) {
      if (Ch == 'U' || Ch == 'u' || Ch == 'L' || Ch == 'l') {
        HasSuffix = true;
        break;
      }
    }
    if (!HasSuffix) {
      ClosingBrace = IsUnsignedLong ? "UL}" : "U}";
    }
  }

  if (IsCallInit) {
    const Expr* InitExprInner{Init->IgnoreImplicit()};
    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExprInner);
        (CCE != nullptr) && CCE->getNumArgs() > 0) {
      if (wouldBraceInitChangeConstructor(CCE)) {
        return;
      }
      const Expr* Arg{CCE->getArg(0)};
      const auto ArgStart = Arg->getBeginLoc();
      const auto ParenCloseLoc = CCE->getParenOrBraceRange().getEnd();
      diag(VarDeclPtr->getLocation(), "variable should use '{}' syntax for initialization")
          << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, ArgStart), "{")
          << FixItHint::CreateReplacement(ParenCloseLoc, ClosingBrace);
    }
  } else {
    diag(VarDeclPtr->getLocation(), "variable should use '{}' syntax for initialization")
        << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarEndLoc, InitStartLoc), "{")
        << FixItHint::CreateInsertion(InitEnd, ClosingBrace);
  }
}

void LintCodeCheck::checkUnsignedSuffix(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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

  auto& SrcMgr = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  const auto* Init = VarDeclPtr->getInit();
  if (Init == nullptr) {
    return;
  }

  const auto* IntLit = dyn_cast<IntegerLiteral>(Init);
  if (IntLit == nullptr) {
    return;
  }

  const auto InitRange = Init->getSourceRange();
  const SourceLocation InitBeginLoc = Init->getBeginLoc();
  const SourceLocation InitEndLoc = Init->getEndLoc();
  const SourceLocation TokEndLoc = Lexer::getLocForEndOfToken(InitEndLoc, 0, SrcMgr, LangOpts);
  auto ValueText =
      Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SrcMgr, LangOpts);

  if (ValueText.empty() || ValueText.size() > kMaxValueTextLength) {
    return;
  }

  std::string FullText = ValueText.str();

  char SuffixBuf[16] = {};
  const char* ExtraBuf = SrcMgr.getCharacterData(InitEndLoc);
  const char* P = ExtraBuf;
  const char* End = P + static_cast<int>(ValueText.size()) + 8;
  for (char SuffixChar : FullText) {
    (void)SuffixChar;
    P++;
  }
  std::size_t SuffixLen = 0;
  while (P < End && SuffixLen < sizeof(SuffixBuf) - 1) {
    if (*P == 'U' || *P == 'u' || *P == 'L' || *P == 'l') {
      SuffixBuf[SuffixLen++] = *P;
      P++;
    } else if (*P == '\0' || *P == '\n' || *P == ' ' || *P == ',' || *P == ';' || *P == ')') {
      break;
    } else {
      P++;
      break;
    }
  }
  if (SuffixLen > 0) {
    FullText.append(SuffixBuf, SuffixLen);
  }

  bool HasSuffix = false;
  for (const char Ch : FullText) {
    if (Ch == 'U' || Ch == 'u' || Ch == 'L' || Ch == 'l') {
      HasSuffix = true;
      break;
    }
  }

  if (HasSuffix) {
    std::size_t SuffixLength = 0;
    for (std::size_t i = 0; i < FullText.size() && SuffixLength < 4; ++i) {
      const auto RevIdx = FullText.size() - 1 - i;
      const char Ch = FullText[RevIdx];
      if (Ch == 'U' || Ch == 'u' || Ch == 'L' || Ch == 'l') {
        SuffixLength++;
      } else {
        break;
      }
    }

    if (SuffixLength == 0) {
      return;
    }

    const std::size_t NumericLen = FullText.size() - SuffixLength;

    bool HasLowerU = false;
    bool HasLowerL = false;
    for (std::size_t i = NumericLen; i < FullText.size(); ++i) {
      if (FullText[i] == 'u')
        HasLowerU = true;
      if (FullText[i] == 'l')
        HasLowerL = true;
    }

    if (HasLowerU || HasLowerL) {
      std::string UpperSuffix;
      UpperSuffix.reserve(SuffixLength);
      for (std::size_t i = NumericLen; i < FullText.size(); ++i) {
        if (FullText[i] == 'u' || FullText[i] == 'U')
          UpperSuffix += 'U';
        else if (FullText[i] == 'l' || FullText[i] == 'L')
          UpperSuffix += 'L';
      }

      const auto InitBeginDecomp = SrcMgr.getDecomposedLoc(Init->getBeginLoc());
      const auto TokBeginDecomp = SrcMgr.getDecomposedLoc(TokEndLoc);
      const FileID FID = InitBeginDecomp.first;
      const FileID FID2 = TokBeginDecomp.first;
      if (FID == FID2) {
        const SourceLocation SuffixStartLoc =
            Init->getBeginLoc().getLocWithOffset(static_cast<int>(NumericLen));
        diag(Init->getBeginLoc(), "unsigned integer literal should use uppercase 'U'/'L' suffix")
            << FixItHint::CreateReplacement(
                   CharSourceRange::getTokenRange(SourceRange(SuffixStartLoc, TokEndLoc)),
                   UpperSuffix);
        return;
      }
      return;
    }
    return;
  }

  const auto InitEnd = Lexer::getLocForEndOfToken(Init->getEndLoc(), 0, SrcMgr, LangOpts);

  const QualType VarType{VarDeclPtr->getType()};
  const clang::Type* CanonicalType{VarType.getTypePtr()->getCanonicalTypeInternal().getTypePtr()};

  bool NeedsLongSuffix{false};
  if (CanonicalType->isSpecificBuiltinType(clang::BuiltinType::ULong) ||
      CanonicalType->isSpecificBuiltinType(clang::BuiltinType::ULongLong) ||
      CanonicalType->isSpecificBuiltinType(clang::BuiltinType::UInt128)) {
    NeedsLongSuffix = true;
  }

  const std::string Suffix{NeedsLongSuffix ? "UL" : "U"};

  diag(Init->getBeginLoc(), "unsigned integer literal should have '%0' suffix")
      << Suffix << FixItHint::CreateInsertion(InitEnd, Suffix);
}

void LintCodeCheck::checkEqualsBraceInit(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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

  const auto* Init = VarDeclPtr->getInit();
  if (Init == nullptr) {
    return;
  }

  const auto* ILE = dyn_cast<InitListExpr>(Init);
  if (ILE == nullptr) {
    return;
  }

  if (isInsideMacro(VarDeclPtr, Ctx)) {
    return;
  }

  auto& SrcMgr = Ctx->getSourceManager();
  auto LangOpts = Ctx->getLangOpts();

  auto VarEndLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);

  if (isAutoType(VarDeclPtr)) {
    if (ILE->getNumInits() == 1) {
      const Expr* SingleInit{ILE->getInit(0)};
      auto BraceEndLoc = Lexer::getLocForEndOfToken(ILE->getRBraceLoc(), 0, SrcMgr, LangOpts);

      diag(VarDeclPtr->getLocation(),
           "auto type should use '=' assignment instead of brace initialization")
          << FixItHint::CreateReplacement(
                 CharSourceRange::getCharRange(VarEndLoc, BraceEndLoc),
                 " = " + Lexer::getSourceText(
                             CharSourceRange::getTokenRange(SingleInit->getSourceRange()), SrcMgr,
                             LangOpts)
                             .str());
    } else {
      diag(VarDeclPtr->getLocation(),
           "auto type should use '=' assignment instead of brace initialization");
    }
    return;
  }

  if (VarDeclPtr->getInitStyle() != VarDecl::CInit) {
    return;
  }

  SourceLocation BraceStartLoc{};
  if (VarDeclPtr->getType()->isArrayType()) {
    if (auto* TSI = VarDeclPtr->getTypeSourceInfo(); TSI != nullptr) {
      TypeLoc TypeLocRef{TSI->getTypeLoc()};
      while (TypeLocRef.getAs<ArrayTypeLoc>()) {
        const ArrayTypeLoc ArrayTypeLocRef{TypeLocRef.getAs<ArrayTypeLoc>()};
        TypeLocRef = ArrayTypeLocRef.getElementLoc();
        BraceStartLoc = ArrayTypeLocRef.getRBracketLoc().isValid()
                            ? ArrayTypeLocRef.getRBracketLoc()
                            : VarDeclPtr->getLocation();
      }
      if (BraceStartLoc.isInvalid()) {
        BraceStartLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
      }
    } else {
      BraceStartLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
    }
  } else {
    BraceStartLoc = Lexer::getLocForEndOfToken(VarDeclPtr->getLocation(), 0, SrcMgr, LangOpts);
  }

  BraceStartLoc = Lexer::getLocForEndOfToken(BraceStartLoc, 0, SrcMgr, LangOpts);
  auto InitStartLoc = Init->getBeginLoc();

  diag(VarDeclPtr->getLocation(), "initializer should use '{}' syntax instead of '= {}'")
      << FixItHint::CreateReplacement(CharSourceRange::getCharRange(BraceStartLoc, InitStartLoc),
                                      "");
}

} // namespace clang::tidy::codelint
