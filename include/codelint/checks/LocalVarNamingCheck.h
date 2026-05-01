#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::codelint {

class LocalVarNamingCheck : public ClangTidyCheck {
public:
  LocalVarNamingCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }
};

} // namespace clang::tidy::codelint
