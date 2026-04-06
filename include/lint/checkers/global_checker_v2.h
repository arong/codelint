#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <optional>
#include <string>
#include <vector>

namespace codelint {
namespace lint {

class GlobalCheckerV2 : public LintChecker, public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  explicit GlobalCheckerV2(const std::optional<GitScope>& scope = std::nullopt);
  ~GlobalCheckerV2() = default;

  LintResult check(const std::string& filepath) override;

  std::string name() const override {
    return "global";
  }
  std::string description() const override {
    return "Detect global variables";
  }
  std::vector<CheckType> provides() const override {
    return {CheckType::GLOBAL_VARIABLE};
  }

  bool can_fix() const override {
    return false;
  }

  // MatchFinder callback override
  void run(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;

private:
  void registerMatchers(clang::ast_matchers::MatchFinder& Finder);
  void reportGlobalVariable(const clang::VarDecl* VD,
                            const clang::ast_matchers::MatchFinder::MatchResult& Result);
  bool checkIsInSystemHeader(const clang::VarDecl* VD, const clang::ASTContext& Context) const;
  bool isExternWithoutInit(const clang::VarDecl* VD) const;

  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;
};

} // namespace lint
} // namespace codelint
