#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <optional>

namespace codelint {
namespace lint {

class GlobalChecker : public LintChecker, public clang::RecursiveASTVisitor<GlobalChecker> {
public:
  explicit GlobalChecker(const std::optional<GitScope>& scope = std::nullopt);
  ~GlobalChecker() = default;

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

  bool VisitVarDecl(clang::VarDecl* VD);
  void runOnAST(clang::ASTContext* Context);

private:
  clang::ASTContext* Context_ = nullptr;
  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;

  bool isGlobalVariable(clang::VarDecl* VD) const;
  bool isInSystemHeader(clang::VarDecl* VD) const;
  bool isExternDeclaration(clang::VarDecl* VD) const;
  void reportGlobalVariable(clang::VarDecl* VD);
  bool shouldSkipUnmodifiedLine(clang::SourceLocation loc) const;
};

} // namespace lint
} // namespace codelint
