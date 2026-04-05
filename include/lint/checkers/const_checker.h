#pragma once

#include "lint/git_scope.h"
#include "lint/issue_reporter.h"
#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace codelint {
namespace lint {

class ConstChecker : public LintChecker, public clang::RecursiveASTVisitor<ConstChecker> {
public:
  explicit ConstChecker(const std::optional<GitScope>& scope = std::nullopt);
  ~ConstChecker() = default;

  LintResult check(const std::string& filepath) override;

  std::string name() const override {
    return "const";
  }
  std::string description() const override {
    return "Detect variables that could be const";
  }
  std::vector<CheckType> provides() const override {
    return {CheckType::CONST_SUGGESTION};
  }

  bool can_fix() const override {
    return true;
  }
  bool apply_fixes(const std::string& filepath, const std::vector<LintIssue>& issues,
                   std::string& modified_content) override;

  bool VisitVarDecl(clang::VarDecl* VD);
  bool VisitBinaryOperator(clang::BinaryOperator* BO);
  bool VisitUnaryOperator(clang::UnaryOperator* UO);
  bool VisitCallExpr(clang::CallExpr* CE);
  void runOnAST(clang::ASTContext* Context);

private:
  clang::ASTContext* Context_ = nullptr;
  IssueReporter Reporter_;
  LintResult Result_;
  std::optional<GitScope> scope_;

  struct VarInfo {
    std::string name;
    std::string type;
    std::string file;
    int line;
    int column;
    bool is_const = false;
    bool is_constexpr = false;
    bool is_reference = false;
    bool is_pointer = false;
    bool is_parameter = false;
    bool is_member = false;
    bool is_global = false;
    bool is_modified = false;
  };

  std::unordered_map<std::string, VarInfo> variables_;
  std::unordered_set<std::string> modified_vars_;

  bool isInSystemHeader(clang::Decl* D) const;
  bool isBuiltinType(const std::string& type) const;
  std::string makeConstSuggestion(const VarInfo& info) const;
  void analyzeAndReport();
  std::string getVarKey(const clang::VarDecl* VD) const;
  bool shouldSkipUnmodifiedLine(clang::SourceLocation loc) const;
};

} // namespace lint
} // namespace codelint
