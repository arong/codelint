#include "lint/checkers/global_checker.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

namespace codelint {
namespace lint {
GlobalChecker::GlobalChecker(const std::optional<GitScope>& scope) : scope_(scope) {
}

class GlobalASTConsumer : public clang::ASTConsumer {
public:
  explicit GlobalASTConsumer(GlobalChecker* checker) : checker_(checker) {
  }

  void HandleTranslationUnit(clang::ASTContext& Context) override {
    checker_->runOnAST(&Context);
  }

private:
  GlobalChecker* checker_;
};

class GlobalFrontendAction : public clang::ASTFrontendAction {
public:
  explicit GlobalFrontendAction(GlobalChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                        llvm::StringRef InFile) override {
    return std::make_unique<GlobalASTConsumer>(checker_);
  }

private:
  GlobalChecker* checker_;
};

class GlobalFrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
  explicit GlobalFrontendActionFactory(GlobalChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<GlobalFrontendAction>(checker_);
  }

private:
  GlobalChecker* checker_;
};

LintResult GlobalChecker::check(const std::string& filepath) {
  Result_.issues.clear();
  Result_.error_count = 0;
  Result_.warning_count = 0;
  Result_.info_count = 0;
  Result_.hint_count = 0;
  Reporter_.clear();

  std::vector<std::string> args = {
      "-std=c++17", "-x", "c++",
      "-resource-dir=/Library/Developer/CommandLineTools/usr/lib/clang/21"};

#if defined(__APPLE__)
  args.push_back("-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1");
  args.push_back("-I/Library/Developer/CommandLineTools/usr/lib/clang/21/include");
  args.push_back("-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include");
  args.push_back("-I/Library/Developer/CommandLineTools/usr/include");
#else
  args.push_back("-I/usr/include/c++/13");
  args.push_back("-I/usr/include/x86_64-linux-gnu/c++/13");
  args.push_back("-I/usr/include");
  args.push_back("-I/usr/local/include");
#endif

  auto compilations = std::make_unique<clang::tooling::FixedCompilationDatabase>(".", args);

  std::vector<std::string> sources = {filepath};
  clang::tooling::ClangTool tool(*compilations, sources);

  GlobalFrontendActionFactory factory(this);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

void GlobalChecker::runOnAST(clang::ASTContext* Context) {
  Context_ = Context;

  clang::TranslationUnitDecl* TU = Context->getTranslationUnitDecl();
  TraverseDecl(TU);
}

bool GlobalChecker::VisitVarDecl(clang::VarDecl* VD) {
  if (!VD || !Context_) {
    return true;
  }

  if (isGlobalVariable(VD) && !isInSystemHeader(VD) && !isExternDeclaration(VD)) {
    reportGlobalVariable(VD);
  }

  return true;
}

bool GlobalChecker::isGlobalVariable(clang::VarDecl* VD) const {
  if (!VD) {
    return false;
  }

  if (clang::isa<clang::ParmVarDecl>(VD)) {
    return false;
  }

  if (VD->isLocalVarDecl() || VD->isStaticLocal()) {
    return false;
  }

  clang::DeclContext* DC = VD->getDeclContext();
  if (clang::isa<clang::RecordDecl>(DC) || clang::isa<clang::ClassTemplateDecl>(DC)) {
    return false;
  }

  if (!VD->hasGlobalStorage()) {
    return false;
  }

  if (VD->isFileVarDecl()) {
    return true;
  }

  clang::DeclContext* parentDC = VD->getLexicalDeclContext();
  while (parentDC) {
    if (clang::isa<clang::TranslationUnitDecl>(parentDC)) {
      return true;
    }
    if (clang::isa<clang::FunctionDecl>(parentDC) || clang::isa<clang::RecordDecl>(parentDC)) {
      return false;
    }
    parentDC = parentDC->getParent();
  }

  return false;
}

bool GlobalChecker::isInSystemHeader(clang::VarDecl* VD) const {
  if (!VD || !Context_) {
    return false;
  }

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();

  clang::FileID fileID = SM.getFileID(loc);
  if (fileID.isInvalid()) {
    return true;
  }

  auto fileEntryRef = SM.getFileEntryRefForID(fileID);
  if (!fileEntryRef) {
    return true;
  }

  std::string filename = fileEntryRef->getName().str();
  // Check for system headers on macOS and Linux
  return filename.find("/usr/include/") != std::string::npos ||
         filename.find("/usr/lib/") != std::string::npos ||
         filename.find("/usr/local/include/") != std::string::npos ||
         filename.find("/Library/Developer/") != std::string::npos ||
         filename.empty();
}

bool GlobalChecker::isExternDeclaration(clang::VarDecl* VD) const {
  if (!VD) {
    return false;
  }

  if (VD->getStorageClass() == clang::SC_Extern) {
    if (!VD->hasInit()) {
      return true;
    }
  }

  return false;
}

void GlobalChecker::reportGlobalVariable(clang::VarDecl* VD) {
  if (!VD || !Context_) {
    return;
  }

  std::string name = VD->getName().str();
  if (name.empty()) {
    return;
  }

  clang::QualType type = VD->getType();
  std::string type_str = type.getAsString();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = SM.getExpansionLineNumber(loc);
  int column = SM.getExpansionColumnNumber(loc);

  LintIssue issue;
  issue.type = CheckType::GLOBAL_VARIABLE;
  issue.severity = Severity::INFO;
  issue.checker_name = "global";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Global variable detected";
  issue.suggestion = "Consider using a singleton or dependency injection pattern";
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

} // namespace lint
} // namespace codelint
