#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <memory>
#include <optional>

namespace codelint {
namespace lint {

class SingletonCheckerV2 : public LintChecker,
                           public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  explicit SingletonCheckerV2(const std::optional<GitScope>& scope = std::nullopt);
  ~SingletonCheckerV2() = default;

  LintResult check(const std::string& filepath) override;

  std::string name() const override {
    return "singleton";
  }
  std::string description() const override {
    return "Detect Meyer's Singleton pattern (static local variable in function returning "
           "reference)";
  }
  std::vector<CheckType> provides() const override {
    return {CheckType::SINGLETON_PATTERN};
  }
  bool can_fix() const override {
    return false;
  }

  void run(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;

private:
  void registerMatchers(clang::ast_matchers::MatchFinder& Finder);
  void reportSingletonPattern(const clang::FunctionDecl* FD, const std::string& staticVarName,
                              clang::ASTContext* Context);
  bool isInSystemHeader(const clang::Decl* D, clang::ASTContext* Context) const;

  clang::ASTContext* Context_ = nullptr;
  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;
  std::unique_ptr<clang::ast_matchers::MatchFinder> Finder_;
};

} // namespace lint
} // namespace codelint
