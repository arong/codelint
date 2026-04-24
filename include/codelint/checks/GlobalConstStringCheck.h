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
  /// Check if the type is std::basic_string<char> (std::string).
  /// Returns false for std::wstring, std::u16string, etc.
  static bool isStdBasicStringChar(QualType QT);

  /// Recursively find a StringLiteral in the initializer expression tree.
  /// Returns nullptr if the initializer is not a string literal
  /// (e.g., function call, concatenation, runtime construction).
  static const StringLiteral* findStringLiteral(const Expr* Init);
};

} // namespace clang::tidy::codelint
