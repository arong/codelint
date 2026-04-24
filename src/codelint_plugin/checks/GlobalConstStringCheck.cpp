#include "codelint/checks/GlobalConstStringCheck.h"
#include "codelint/utils/InitUtils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace clang::tidy::codelint {

using namespace ast_matchers;
using utils::isInsideMacro;
using utils::isInSystemHeader;
using utils::shouldSkipExtern;

const StringLiteral* GlobalConstStringCheck::findStringLiteral(const Expr* Init) {
  if (Init == nullptr) {
    return nullptr;
  }

  const Expr* E = Init->IgnoreImplicit();

  if (isa<StringLiteral>(E)) {
    return cast<StringLiteral>(E);
  }

  if (const auto* CCE = dyn_cast<CXXConstructExpr>(E); CCE != nullptr) {
    if (CCE->getNumArgs() == 1) {
      return findStringLiteral(CCE->getArg(0));
    }
    return nullptr;
  }

  if (const auto* Cast = dyn_cast<CXXFunctionalCastExpr>(E); Cast != nullptr) {
    return findStringLiteral(Cast->getSubExpr());
  }

  if (const auto* MatTemp = dyn_cast<MaterializeTemporaryExpr>(E); MatTemp != nullptr) {
    return findStringLiteral(MatTemp->getSubExpr());
  }

  if (const auto* BindTemp = dyn_cast<CXXBindTemporaryExpr>(E); BindTemp != nullptr) {
    return findStringLiteral(BindTemp->getSubExpr());
  }

  if (const auto* Cleanups = dyn_cast<ExprWithCleanups>(E); Cleanups != nullptr) {
    return findStringLiteral(Cleanups->getSubExpr());
  }

  return nullptr;
}

void GlobalConstStringCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(varDecl(hasGlobalStorage(), unless(isStaticLocal()), unless(parmVarDecl()),
                             unless(hasAncestor(recordDecl())))
                         .bind("globalConstVar"),
                     this);
}

void GlobalConstStringCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }
  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const auto* VD = Result.Nodes.getNodeAs<VarDecl>("globalConstVar");
  if (VD == nullptr || VD->isImplicit() || VD->getName().empty()) {
    return;
  }

  if (isInSystemHeader(VD->getLocation(), Result.Context)) {
    return;
  }
  if (isInsideMacro(VD, Result.Context)) {
    return;
  }
  if (shouldSkipExtern(VD)) {
    return;
  }

  if (!VD->getType().isConstQualified()) {
    return;
  }

  if (VD->isConstexpr()) {
    return;
  }

  const auto* CXXRD = VD->getType()->getAsCXXRecordDecl();
  if (CXXRD == nullptr) {
    return;
  }

  if (CXXRD->getName() != "basic_string") {
    return;
  }

  bool InStdNamespace = false;
  for (const DeclContext* Ctx = CXXRD->getDeclContext(); Ctx != nullptr; Ctx = Ctx->getParent()) {
    if (const auto* NS = dyn_cast<NamespaceDecl>(Ctx); NS != nullptr && NS->isStdNamespace()) {
      InStdNamespace = true;
      break;
    }
  }
  if (!InStdNamespace) {
    return;
  }

  const auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD);
  if (CTSD == nullptr) {
    return;
  }

  const auto& Args = CTSD->getTemplateArgs();
  if (Args.size() < 1) {
    return;
  }
  const Type* CharTy = Args[0].getAsType().getTypePtr();
  if (CharTy == nullptr || (!CharTy->isSpecificBuiltinType(BuiltinType::Char_U) &&
                            !CharTy->isSpecificBuiltinType(BuiltinType::Char_S))) {
    return;
  }

  const Expr* Init = VD->getInit();
  if (Init == nullptr) {
    return;
  }

  const StringLiteral* SL = findStringLiteral(Init);
  if (SL == nullptr) {
    return;
  }

  auto& SrcMgr = Result.Context->getSourceManager();
  auto LangOpts = Result.Context->getLangOpts();

  const SourceLocation VarStart = VD->getBeginLoc();
  const SourceLocation VarNameStart = VD->getLocation();
  const SourceLocation VarNameEnd =
      Lexer::getLocForEndOfToken(VD->getLocation(), 0, SrcMgr, LangOpts);

  const auto LitText =
      Lexer::getSourceText(CharSourceRange::getTokenRange(SL->getSourceRange()), SrcMgr, LangOpts);

  const std::string NewInit = "{" + LitText.str() + "}";

  auto Diag =
      diag(VD->getLocation(), "global const std::string initialized with string literal should be "
                              "'constexpr const char*'");

  Diag << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarStart, VarNameStart),
                                       "constexpr const char* ");

  const SourceLocation InitEnd = Lexer::getLocForEndOfToken(Init->getEndLoc(), 0, SrcMgr, LangOpts);
  Diag << FixItHint::CreateReplacement(CharSourceRange::getCharRange(VarNameEnd, InitEnd), NewInit);
}

} // namespace clang::tidy::codelint
