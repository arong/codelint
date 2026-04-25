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

  diag(VD->getLocation(), "D0: matched '%0'") << VD->getName();

  if (isInSystemHeader(VD->getLocation(), Result.Context)) {
    diag(VD->getLocation(), "D1: system header");
    return;
  }
  if (isInsideMacro(VD, Result.Context)) {
    diag(VD->getLocation(), "D2: macro");
    return;
  }
  if (shouldSkipExtern(VD)) {
    diag(VD->getLocation(), "D3: extern");
    return;
  }

  if (!VD->getType().isConstQualified()) {
    diag(VD->getLocation(), "D4: not const");
    return;
  }

  if (VD->isConstexpr()) {
    diag(VD->getLocation(), "D5: constexpr");
    return;
  }

  const auto* CXXRD = VD->getType()->getAsCXXRecordDecl();
  if (CXXRD == nullptr) {
    diag(VD->getLocation(), "D6: no CXXRD");
    return;
  }

  diag(VD->getLocation(), "D7: CXXRD='%0'") << CXXRD->getName();

  if (CXXRD->getName() != "basic_string") {
    diag(VD->getLocation(), "D8: not basic_string");
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
    diag(VD->getLocation(), "D9: not std namespace");
    return;
  }

  const auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD);
  if (CTSD == nullptr) {
    diag(VD->getLocation(), "D10: no CTSD");
    return;
  }

  const auto& Args = CTSD->getTemplateArgs();
  if (Args.size() < 1) {
    diag(VD->getLocation(), "D11: no args");
    return;
  }
  const Type* CharTy = Args[0].getAsType().getTypePtr();
  if (CharTy == nullptr || (!CharTy->isSpecificBuiltinType(BuiltinType::Char_U) &&
                            !CharTy->isSpecificBuiltinType(BuiltinType::Char_S))) {
    diag(VD->getLocation(), "D12: not char");
    return;
  }

  const Expr* Init = VD->getInit();
  if (Init == nullptr) {
    diag(VD->getLocation(), "D13: no init");
    return;
  }

  diag(VD->getLocation(), "D14: init type='%0'") << Init->IgnoreImplicit()->getStmtClassName();

  const StringLiteral* SL = findStringLiteral(Init);
  if (SL == nullptr) {
    diag(VD->getLocation(), "D15: no string literal");
    return;
  }

  diag(VD->getLocation(), "D16: found SL, emitting fix");

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
