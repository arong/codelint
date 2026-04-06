#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <memory>
#include <optional>

namespace codelint {
namespace lint {

class InitCheckerV2 : public LintChecker, public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  explicit InitCheckerV2(const std::optional<GitScope>& scope = std::nullopt);
  ~InitCheckerV2() = default;

  LintResult check(const std::string& filepath) override;

  std::string name() const override {
    return "init";
  }
  std::string description() const override {
    return "Check variable initialization and suggest improvements";
  }
  std::vector<CheckType> provides() const override {
    return {CheckType::UNINITIALIZED_VAR, CheckType::INIT_SYNTAX, CheckType::INIT_UNSIGNED_SUFFIX};
  }
  bool can_fix() const override {
    return false;
  }

  void run(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;

private:
  void registerMatchers(clang::ast_matchers::MatchFinder& Finder);

  void handleUninitialized(const clang::VarDecl* VD, clang::ASTContext* Ctx);
  void handleEqualsSyntax(const clang::VarDecl* VD, clang::ASTContext* Ctx);
  void handleUnsignedSuffix(const clang::VarDecl* VD, clang::ASTContext* Ctx);

  bool shouldSkipVarDecl(const clang::VarDecl* VD, clang::ASTContext* Ctx) const;
  bool shouldSkipForLoop(const clang::VarDecl* VD, clang::ASTContext* Ctx) const;
  bool shouldSkipAuto(const clang::VarDecl* VD) const;
  bool shouldSkipLambda(const clang::VarDecl* VD, clang::ASTContext* Ctx) const;
  bool isInSystemHeader(const clang::VarDecl* VD, clang::ASTContext* Ctx) const;

  bool needsUnsignedSuffix(const clang::VarDecl* VD, clang::ASTContext* Ctx) const;
  bool hasSuffixAlready(const clang::IntegerLiteral* literal, clang::ASTContext* Ctx) const;

  clang::ASTContext* Context_ = nullptr;
  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;
  std::unique_ptr<clang::ast_matchers::MatchFinder> Finder_;
};

} // namespace lint
} // namespace codelint
