#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <optional>

namespace clang::tidy::codelint {

class LintCodeCheck : public ClangTidyCheck {
public:
  LintCodeCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void checkEqualsInit(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkUnsignedSuffix(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkEqualsBraceInit(const VarDecl* VarDeclPtr, ASTContext* Ctx);

  static bool wouldBraceInitChangeConstructor(const CXXConstructExpr* CCE);
  static std::optional<bool>
  wouldBraceInitChangeBasicStringConstructor(const CXXConstructExpr* CCE,
                                             const CXXRecordDecl* Record);
};

} // namespace clang::tidy::codelint
