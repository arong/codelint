#pragma once

#include "codelint/Compatibility.h"
#include <clang-tidy/ClangTidyCheck.h>
#include <string>

namespace clang::tidy::codelint {

class StrictBoolConditionCheck : public ClangTidyCheck {
public:
  StrictBoolConditionCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus14 && !CODELINT_LANGOPTS_IS_CPP23(LangOpts);
  }

private:
  void checkCondition(const Expr* Cond, ASTContext* Ctx);
  static bool isBoolType(const Expr* expr);
  static const Expr* getNonBoolOperand(const Expr* expr);
  static std::string getComparisonFix(const Expr* expr);
};

} // namespace clang::tidy::codelint
