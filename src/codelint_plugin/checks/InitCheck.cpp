#include "codelint/checks/InitCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>

namespace clang::tidy::codelint {

// Maximum length for value text extraction to avoid pathological cases
constexpr std::size_t kMaxValueTextLength = 64;

bool InitCheck::isInSystemHeader(const SourceLocation Loc, ASTContext* Ctx) {
  if (Ctx == nullptr) {
    return false;
  }
  const auto& SrcMgr = Ctx->getSourceManager();
  const SourceLocation ExpansionLoc{SrcMgr.getExpansionLoc(Loc)};
  return SrcMgr.isInSystemHeader(ExpansionLoc);
}

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
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<clang::VarDecl>("uninit");
      VarDeclPtr != nullptr) {
    checkUninitialized(VarDeclPtr, Result.Context);
  } else if (const auto* FieldDeclPtr = Result.Nodes.getNodeAs<clang::FieldDecl>("uninit_field");
             FieldDeclPtr != nullptr) {
    checkUninitializedField(FieldDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<clang::VarDecl>("equals");
             VarDeclPtr != nullptr) {
    checkEqualsInit(VarDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<clang::VarDecl>("unsigned");
             VarDeclPtr != nullptr) {
    checkUnsignedSuffix(VarDeclPtr, Result.Context);
  } else if (const auto* VarDeclPtr = Result.Nodes.getNodeAs<clang::VarDecl>("equals_brace");
             VarDeclPtr != nullptr) {
    checkEqualsBraceInit(VarDeclPtr, Result.Context);
  } else if (const auto* Ctor = Result.Nodes.getNodeAs<clang::CXXConstructorDecl>("constructor");
             Ctor != nullptr) {
    checkUninitializedMemberVariablesInConstructors(Ctor, Result.Context);
  }
}

bool InitCheck::shouldSkipAuto(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  // Check if type contains auto (including auto*, const auto*, auto&, etc.)
  // getContainedAutoType() returns the AutoType if the type uses auto deduction,
  // even if the top-level type is a pointer or reference to auto.
  return VarDeclPtr->getType()->getContainedAutoType() != nullptr;
}

bool InitCheck::isAutoType(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  // Use TypeSourceInfo to detect auto in the source code
  // Even after deduction, getContainedAutoTypeLoc() returns the AutoTypeLoc
  // if the variable was declared with auto
  if (const auto* TSI = VarDeclPtr->getTypeSourceInfo(); TSI != nullptr) {
    const AutoTypeLoc AutoTypeLocRef{TSI->getTypeLoc().getContainedAutoTypeLoc()};
    return !AutoTypeLocRef.isNull();
  }
  return false;
}

bool InitCheck::shouldSkipUnion(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  const auto* DeclCtx = VarDeclPtr->getDeclContext();
  if (const auto* RecordDeclPtr = dyn_cast<RecordDecl>(DeclCtx); RecordDeclPtr != nullptr) {
    return RecordDeclPtr->isUnion();
  }
  return false;
}

bool InitCheck::shouldSkipExtern(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  return VarDeclPtr->getStorageClass() == SC_Extern;
}

bool InitCheck::hasExplicitInitializer(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }

  if (!VarDeclPtr->hasInit()) {
    return false;
  }

  const auto InitStyle = VarDeclPtr->getInitStyle();
  if (InitStyle == VarDecl::CInit || InitStyle == VarDecl::ListInit) {
    return true;
  }

  const Expr* Init{VarDeclPtr->getInit()};
  if (Init == nullptr) {
    return false;
  }

  const Expr* InitExpr{Init->IgnoreImplicit()};

  if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr); CCE != nullptr) {
    if (CCE->isListInitialization()) {
      return true;
    }
    if (CCE->getNumArgs() > 0) {
      return true;
    }
    return false;
  }

  if (const auto* TOE = dyn_cast<CXXTemporaryObjectExpr>(InitExpr); TOE != nullptr) {
    return true;
  }

  return false;
}

bool InitCheck::shouldSkipEnumClass(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  if (!VarDeclPtr->getType()->isScopedEnumeralType()) {
    return false;
  }
  return !isEnumZeroValidType(VarDeclPtr->getType().getTypePtr());
}

bool InitCheck::isEnumZeroValidType(const Type* TypePtr) {
  if ((TypePtr == nullptr) || !TypePtr->isEnumeralType()) {
    return false;
  }

  const EnumDecl* Enum{nullptr};
  if (const auto* EnumTypePtr = TypePtr->getAs<EnumType>(); EnumTypePtr != nullptr) {
    Enum = EnumTypePtr->getDecl();
  } else if (const auto* ElabTypePtr = TypePtr->getAs<ElaboratedType>(); ElabTypePtr != nullptr) {
    if (const auto* InnerEnumTypePtr = ElabTypePtr->getNamedType()->getAs<EnumType>();
        InnerEnumTypePtr != nullptr) {
      Enum = InnerEnumTypePtr->getDecl();
    }
  }

  if (Enum == nullptr) {
    return false;
  }

  return llvm::any_of(Enum->enumerators(),
                      [](const auto* EnumConst) { return EnumConst->getInitVal().isZero(); });
}

bool InitCheck::hasExplicitInitializer(const FieldDecl* FieldDeclPtr) {
  if (FieldDeclPtr == nullptr) {
    return false;
  }

  if (!FieldDeclPtr->hasInClassInitializer()) {
    return false;
  }

  return true;
}

bool InitCheck::isInsideMacro(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
  if ((VarDeclPtr == nullptr) || (Ctx == nullptr)) {
    return false;
  }

  const auto& SrcMgr = Ctx->getSourceManager();
  const SourceLocation Loc{VarDeclPtr->getLocation()};

  return SrcMgr.isMacroBodyExpansion(Loc) || SrcMgr.isMacroArgExpansion(Loc);
}

bool InitCheck::wouldBraceInitChangeConstructor(const CXXConstructExpr* CCE) {
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

  llvm::errs() << "DEBUG wouldBraceInit: record='" << Record->getName() << "' qualified='"
               << Record->getQualifiedNameAsString() << "' numArgs=" << CCE->getNumArgs() << "\n";

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
          llvm::errs() << "DEBUG: Found initializer_list constructor\n";
          auto TemplateArgs = TST->template_arguments();
          if (!TemplateArgs.empty()) {
            InitListElemType = TemplateArgs[0].getAsType();
            llvm::errs() << "DEBUG: InitListElemType='" << InitListElemType.getAsString() << "'\n";
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
    llvm::errs() << "DEBUG: No initializer_list ctor, returning false\n";
    return false;
  }

  const unsigned NumArgs = CCE->getNumArgs();

  if (NumArgs >= 2) {
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
    const Expr* ArgExpr = CCE->getArg(0);
    const Type* ArgTy = ArgExpr->getType().getTypePtr();

    llvm::errs() << "DEBUG: NumArgs=1, ArgTy='" << ArgTy->getTypeClassName() << "' asString='"
                 << ArgExpr->getType().getAsString() << "'\n";

    // Special case: std::basic_string with string literal argument
    // Brace init would NOT change constructor selection:
    //   std::string("hello") and std::string{"hello"} use the same const char* constructor
    // On different STL implementations, the argument might be:
    //   - PointerType (const char* after decay)
    //   - ArrayType (const char[N] before decay, e.g., StringLiteral)
    //   - StringLiteral directly
    const auto RecordName = Record->getName();
    llvm::errs() << "DEBUG: RecordName='" << RecordName << "'\n";
    if (RecordName == "basic_string") {
      llvm::errs() << "DEBUG: basic_string special case entered\n";
      // Helper to check if a type is char or wchar_t
      auto isCharOrWCharType = [](const Type* Ty) {
        if (Ty == nullptr) {
          return false;
        }
        // Check for char types (plain char, signed char, unsigned char)
        if (Ty->isSpecificBuiltinType(BuiltinType::Char_U) ||
            Ty->isSpecificBuiltinType(BuiltinType::Char_S) ||
            Ty->isSpecificBuiltinType(BuiltinType::UChar) ||
            Ty->isSpecificBuiltinType(BuiltinType::SChar) ||
            Ty->isSpecificBuiltinType(BuiltinType::Char8)) {
          return true;
        }
        // Check for wchar_t types
        if (Ty->isSpecificBuiltinType(BuiltinType::WChar_U) ||
            Ty->isSpecificBuiltinType(BuiltinType::WChar_S)) {
          return true;
        }
        return false;
      };

      // Case 1: Argument is pointer to char/wchar_t
      if (ArgTy->isPointerType()) {
        const Type* PointeeTy = ArgTy->getPointeeType().getTypePtr();
        llvm::errs() << "DEBUG: Case 1 - PointerType, pointee='"
                     << (PointeeTy ? PointeeTy->getTypeClassName() : "null") << "'\n";
        if (isCharOrWCharType(PointeeTy)) {
          llvm::errs() << "DEBUG: Case 1 matched, returning false\n";
          return false;
        }
      }

      // Case 2: Argument is array of char/wchar_t (StringLiteral before decay)
      if (const ArrayType* ArrTy = ArgTy->getAsArrayTypeUnsafe(); ArrTy != nullptr) {
        const Type* ElemTy = ArrTy->getElementType().getTypePtr();
        llvm::errs() << "DEBUG: Case 2 - ArrayType, elem='"
                     << (ElemTy ? ElemTy->getTypeClassName() : "null") << "'\n";
        if (isCharOrWCharType(ElemTy)) {
          llvm::errs() << "DEBUG: Case 2 matched, returning false\n";
          return false;
        }
      }

      // Case 3: Argument is or contains a StringLiteral
      // Strip implicit casts and check the underlying expression
      const Expr* ArgWithoutImplicit = ArgExpr->IgnoreImplicit();
      llvm::errs() << "DEBUG: Case 3 - ArgWithoutImplicit type='"
                   << ArgWithoutImplicit->getType().getAsString()
                   << "' isStringLiteral=" << isa<StringLiteral>(ArgWithoutImplicit) << "\n";
      if (isa<StringLiteral>(ArgWithoutImplicit)) {
        llvm::errs() << "DEBUG: Case 3a matched (StringLiteral), returning false\n";
        return false;
      }
      // Also check the type of the stripped expression (might be array type)
      const Type* ArgWithoutImplicitTy = ArgWithoutImplicit->getType().getTypePtr();
      if (const ArrayType* ArrTy = ArgWithoutImplicitTy->getAsArrayTypeUnsafe(); ArrTy != nullptr) {
        const Type* ElemTy = ArrTy->getElementType().getTypePtr();
        llvm::errs() << "DEBUG: Case 3b - ArrayType after IgnoreImplicit, elem='"
                     << (ElemTy ? ElemTy->getTypeClassName() : "null") << "'\n";
        if (isCharOrWCharType(ElemTy)) {
          llvm::errs() << "DEBUG: Case 3b matched, returning false\n";
          return false;
        }
      }
    }

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

bool InitCheck::hasNonTrivialDefaultConstructor(QualType QualTypeRef) {
  if (QualTypeRef.isNull()) {
    return false;
  }

  if (QualTypeRef->isArrayType()) {
    const Type* ElemTypePtr{QualTypeRef->getArrayElementTypeNoTypeQual()};
    return hasNonTrivialDefaultConstructor(QualType(ElemTypePtr, 0));
  }

  if (const auto* RecordDeclPtr = QualTypeRef->getAsCXXRecordDecl(); RecordDeclPtr != nullptr) {
    if (RecordDeclPtr->hasDefinition()) {
      return RecordDeclPtr->hasNonTrivialDefaultConstructor();
    }
  }

  return false;
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
      !isEnumZeroValidType(FieldDeclPtr->getType().getTypePtr())) {
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

// initialization edge cases
void InitCheck::checkUninitialized(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
  if ((VarDeclPtr == nullptr) || (Ctx == nullptr)) {
    return;
  }

  const auto Name = VarDeclPtr->getName();
  if (Name.empty()) {
    return;
  }

  // Skip checking if variable is inside a macro
  if (isInsideMacro(VarDeclPtr, Ctx)) {
    return;
  }

  if (shouldSkipAuto(VarDeclPtr) || shouldSkipUnion(VarDeclPtr) || shouldSkipExtern(VarDeclPtr) ||
      shouldSkipEnumClass(VarDeclPtr)) {
    return;
  }

  if (isInSystemHeader(VarDeclPtr->getLocation(), Ctx)) {
    return;
  }

  if (hasExplicitInitializer(VarDeclPtr)) {
    return;
  }

  // Check if this is a reference type - needs special handling since references must be initialized
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
      // subsequent tokens)
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

  // Check if this is a C-style array and provide a more specific message
  // Different diagnostic messages for arrays vs non-arrays with same fix-it logic
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

// list constructors and type conversions
void InitCheck::checkEqualsInit(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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

  if (InitStyle == VarDecl::CInit) {
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

      const CXXConstructorDecl* Ctor = CCE->getConstructor();
      const CXXRecordDecl* Record = Ctor ? Ctor->getParent() : nullptr;
      llvm::errs() << "DEBUG: var='" << Name << "' type='" << VarDeclPtr->getType().getAsString()
                   << "' record='" << (Record ? Record->getName() : "null")
                   << "' wouldBraceInit=" << wouldBraceInitChangeConstructor(CCE) << "\n";

      if (wouldBraceInitChangeConstructor(CCE)) {
        llvm::errs() << "DEBUG: SKIPPING '" << Name << "' due to wouldBraceInitChangeConstructor\n";
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

  if (const Expr* InitExpr{Init->IgnoreImplicit()};
      (IsUnsignedInt || IsUnsignedLong) && isa<IntegerLiteral>(InitExpr)) {
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
    const Expr* InitExpr{Init->IgnoreImplicit()};
    if (const auto* CCE = dyn_cast<CXXConstructExpr>(InitExpr);
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

void InitCheck::checkUnsignedSuffix(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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
  auto ValueText =
      Lexer::getSourceText(CharSourceRange::getTokenRange(InitRange), SrcMgr, LangOpts);

  if (ValueText.empty() || ValueText.size() > kMaxValueTextLength) {
    return;
  }

  for (const char Ch : ValueText) {
    if (Ch == 'U' || Ch == 'u' || Ch == 'L' || Ch == 'l') {
      return;
    }
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

// brace initialization
void InitCheck::checkEqualsBraceInit(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
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

  // For auto types, brace initialization should use '=' assignment instead
  // e.g., auto x{1} or auto x = {1} should become auto x = 1
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

  // For non-auto types with CInit style (= {}), suggest removing '=' to use direct brace init
  // e.g., int x = {1} should become int x{1}
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

// multiple conditions
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

  // Collection to store member variables that are not initialized in this constructor
  constexpr std::size_t kInitialMemberCapacity = 16;
  llvm::SmallVector<const FieldDecl*, kInitialMemberCapacity> UninitializedMembers{};

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

  // Report diagnostics for uninitialized members
  for (const FieldDecl* Field : UninitializedMembers) {
    // Skip fields in system headers
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
