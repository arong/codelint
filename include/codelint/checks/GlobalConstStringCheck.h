#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::codelint {

class GlobalConstStringCheck : public ClangTidyCheck {
public:
  GlobalConstStringCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  static const StringLiteral* findStringLiteral(const Expr* Init);
};

} // namespace clang::tidy::codelint
