#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <optional>

namespace codelint {
namespace lint {

class SingletonChecker : public LintChecker, public clang::RecursiveASTVisitor<SingletonChecker> {
public:
  explicit SingletonChecker(const std::optional<GitScope>& scope = std::nullopt);
  ~SingletonChecker() = default;

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

  bool VisitFunctionDecl(clang::FunctionDecl* FD);
  void runOnAST(clang::ASTContext* Context);

private:
  clang::ASTContext* Context_ = nullptr;
  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;

  bool returnsReference(clang::FunctionDecl* FD) const;
  bool hasMeyersSingletonPattern(clang::FunctionDecl* FD, std::string& staticVarName) const;
  bool isInSystemHeader(clang::Decl* D) const;
  void reportSingletonPattern(clang::FunctionDecl* FD, const std::string& staticVarName);
};

} // namespace lint
} // namespace codelint
