#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy {
namespace codelint {

class SingletonCheck : public ClangTidyCheck {
public:
  SingletonCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }
};

} // namespace codelint
} // namespace clang::tidy
