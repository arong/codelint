#include "lint/lint_action.h"

#include <memory>

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

namespace codelint {
namespace lint {

// IssueReporter stub - will be implemented in later task
class IssueReporter {
public:
  IssueReporter() = default;
  ~IssueReporter() = default;
};

// LintVisitor stub - will be implemented in later task
class LintVisitor {
public:
  explicit LintVisitor(clang::ASTContext& Context) : Context_(Context) {
  }
  ~LintVisitor() = default;

  bool TraverseDecl(clang::Decl* D) {
    return true;
  }

private:
  clang::ASTContext& Context_;
};

// ============ LintConsumer Implementation ============

LintConsumer::LintConsumer(clang::ASTContext& Context) {
  visitor_ = std::make_unique<LintVisitor>(Context);
  reporter_ = std::make_unique<IssueReporter>();
}

LintConsumer::~LintConsumer() = default;

void LintConsumer::HandleTranslationUnit(clang::ASTContext& Context) {
  if (visitor_) {
    visitor_->TraverseDecl(Context.getTranslationUnitDecl());
  }
}

// ============ LintAction Implementation ============

LintAction::~LintAction() = default;

std::unique_ptr<clang::ASTConsumer> LintAction::CreateASTConsumer(clang::CompilerInstance& CI,
                                                                  llvm::StringRef InFile) {
  current_filename_ = InFile.str();
  return std::make_unique<LintConsumer>(CI.getASTContext());
}

bool LintAction::BeginSourceFileAction(clang::CompilerInstance& CI) {
  return true;
}

void LintAction::EndSourceFileAction() {
}

} // namespace lint
} // namespace codelint
