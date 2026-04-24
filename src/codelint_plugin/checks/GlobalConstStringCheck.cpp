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

  return nullptr;
}

void GlobalConstStringCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  const auto IsStdString = hasType(hasUnqualifiedDesugaredType(
      recordType(hasDeclaration(cxxRecordDecl(hasName("::std::basic_string"))))));

  Finder->addMatcher(varDecl(hasGlobalStorage(), IsStdString, hasType(isConstQualified()),
                             unless(isConstexpr()), unless(parmVarDecl()),
                             unless(hasAncestor(functionDecl())), unless(hasAncestor(recordDecl())),
                             unless(isStaticLocal()), hasInitializer(expr()))
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

  // Only handle std::basic_string<char>, not wchar_t/char16_t/char32_t variants
  const auto* CXXRD = VD->getType()->getAsCXXRecordDecl();
  if (CXXRD == nullptr) {
    return;
  }

  // Verify the type is in the std namespace (handles inline namespaces like __cxx11)
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

  if (const auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(CXXRD); CTSD != nullptr) {
    const auto& Args = CTSD->getTemplateArgs();
    if (Args.size() > 0) {
      const Type* CharTy = Args[0].getAsType().getTypePtr();
      if (CharTy == nullptr || (!CharTy->isSpecificBuiltinType(BuiltinType::Char_U) &&
                                !CharTy->isSpecificBuiltinType(BuiltinType::Char_S))) {
        return;
      }
    }
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
