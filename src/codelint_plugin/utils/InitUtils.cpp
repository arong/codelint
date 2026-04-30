#include "codelint/utils/InitUtils.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>

namespace clang::tidy::codelint::utils {

bool isInSystemHeader(SourceLocation Loc, ASTContext* Ctx) {
  if (Ctx == nullptr) {
    return false;
  }
  const auto& SrcMgr = Ctx->getSourceManager();
  const SourceLocation ExpansionLoc{SrcMgr.getExpansionLoc(Loc)};
  return SrcMgr.isInSystemHeader(ExpansionLoc);
}

bool shouldSkipAuto(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  return VarDeclPtr->getType()->getContainedAutoType() != nullptr;
}

bool isAutoType(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  if (const auto* TSI = VarDeclPtr->getTypeSourceInfo(); TSI != nullptr) {
    const AutoTypeLoc AutoTypeLocRef{TSI->getTypeLoc().getContainedAutoTypeLoc()};
    return !AutoTypeLocRef.isNull();
  }
  return false;
}

bool shouldSkipUnion(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  const auto* DeclCtx = VarDeclPtr->getDeclContext();
  if (const auto* RecordDeclPtr = dyn_cast<RecordDecl>(DeclCtx); RecordDeclPtr != nullptr) {
    return RecordDeclPtr->isUnion();
  }
  return false;
}

bool shouldSkipExtern(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  return VarDeclPtr->getStorageClass() == SC_Extern;
}

bool shouldSkipEnumClass(const VarDecl* VarDeclPtr) {
  if (VarDeclPtr == nullptr) {
    return false;
  }
  if (!VarDeclPtr->getType()->isScopedEnumeralType()) {
    return false;
  }
  return !isEnumZeroValidType(VarDeclPtr->getType().getTypePtr());
}

bool isEnumZeroValidType(const Type* TypePtr) {
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

bool hasExplicitInitializer(const VarDecl* VarDeclPtr) {
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

bool hasExplicitInitializer(const FieldDecl* FieldDeclPtr) {
  if (FieldDeclPtr == nullptr) {
    return false;
  }

  if (!FieldDeclPtr->hasInClassInitializer()) {
    return false;
  }

  return true;
}

bool isInsideMacro(const VarDecl* VarDeclPtr, ASTContext* Ctx) {
  if ((VarDeclPtr == nullptr) || (Ctx == nullptr)) {
    return false;
  }

  const auto& SrcMgr = Ctx->getSourceManager();
  const SourceLocation Loc{VarDeclPtr->getLocation()};

  return SrcMgr.isMacroBodyExpansion(Loc) || SrcMgr.isMacroArgExpansion(Loc);
}

bool hasNonTrivialDefaultConstructor(QualType QualTypeRef) {
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

} // namespace clang::tidy::codelint::utils
