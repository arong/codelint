#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::codelint {

class InitCheck : public ClangTidyCheck {
public:
  InitCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void checkUninitialized(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkUninitializedField(const FieldDecl* FieldDeclPtr, ASTContext* Ctx);
  void checkDangerousConversion(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkInitializerListSingleElement(const CXXConstructExpr* CCE, ASTContext* Ctx);
  void checkUninitializedMemberVariablesInConstructors(const CXXConstructorDecl* Ctor,
                                                       ASTContext* Ctx);
};

} // namespace clang::tidy::codelint
