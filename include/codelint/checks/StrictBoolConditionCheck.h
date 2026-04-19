#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::codelint {

class StrictBoolConditionCheck : public ClangTidyCheck {
public:
  StrictBoolConditionCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus14 && !LangOpts.CPlusPlus23;
  }

private:
  void checkCondition(const Expr* Cond, ASTContext* Ctx);
  static bool isBoolType(const Expr* expr);
  static const Expr* getNonBoolOperand(const Expr* expr);
};

} // namespace clang::tidy::codelint
