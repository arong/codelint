#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <string>
#include <vector>

namespace clang::tidy::codelint {

class LogTagMismatchCheck : public ClangTidyCheck {
public:
  LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context);

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;
  void storeOptions(ClangTidyOptions::OptionMap& Opts) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  // Configuration options
  std::string LogMacroNames;
  bool AllowQualifiedName;

  // Helper methods
  const FunctionDecl* getEnclosingFunction(const Stmt* S, ASTContext* Ctx);
  std::vector<std::string> extractTags(StringRef LogText);
  bool isTagValid(StringRef Tag, const FunctionDecl* Func);
  bool matchesMacroPattern(StringRef MacroName);
};

} // namespace clang::tidy::codelint
