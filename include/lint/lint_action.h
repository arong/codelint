#pragma once

#include <memory>
#include <string>

#include <clang/Frontend/FrontendAction.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/Compiler.h>

namespace codelint {
namespace lint {

// Forward declarations
class LintVisitor;
class IssueReporter;

// LintConsumer - AST 处理入口
class LintConsumer : public clang::ASTConsumer {
public:
  explicit LintConsumer(clang::ASTContext& Context);
  ~LintConsumer() override;

  // 处理完整的 translation unit
  void HandleTranslationUnit(clang::ASTContext& Context) override;

private:
  std::unique_ptr<LintVisitor> visitor_;
  std::unique_ptr<IssueReporter> reporter_;
};

// LintAction - 每个 TU 的入口
class LintAction : public clang::ASTFrontendAction {
public:
  LintAction() = default;
  ~LintAction() override;

  // 创建 ASTConsumer 实例
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance& CI,
                    llvm::StringRef InFile) override;

  // Begin source file action (returns true to continue processing)
  bool BeginSourceFileAction(clang::CompilerInstance& CI) override;

  // End source file processing
  void EndSourceFileAction() override;

private:
  std::string current_filename_;
};

}  // namespace lint
}  // namespace codelint
