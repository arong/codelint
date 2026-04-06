#include "lint/checkers/global_checker_v2.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

namespace codelint {
namespace lint {
GlobalCheckerV2::GlobalCheckerV2(const std::optional<GitScope>& scope) : scope_(scope) {
}

class GlobalV2ASTConsumer : public clang::ASTConsumer {
public:
  explicit GlobalV2ASTConsumer(clang::ast_matchers::MatchFinder& Finder) : Finder_(Finder) {
  }

  void HandleTranslationUnit(clang::ASTContext& Context) override {
    Finder_.matchAST(Context);
  }

private:
  clang::ast_matchers::MatchFinder& Finder_;
};

class GlobalV2FrontendAction : public clang::ASTFrontendAction {
public:
  explicit GlobalV2FrontendAction(clang::ast_matchers::MatchFinder& Finder) : Finder_(Finder) {
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                        llvm::StringRef InFile) override {
    return std::make_unique<GlobalV2ASTConsumer>(Finder_);
  }

private:
  clang::ast_matchers::MatchFinder& Finder_;
};

class GlobalV2FrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
  explicit GlobalV2FrontendActionFactory(clang::ast_matchers::MatchFinder& Finder)
      : Finder_(Finder) {
  }

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<GlobalV2FrontendAction>(Finder_);
  }

private:
  clang::ast_matchers::MatchFinder& Finder_;
};

LintResult GlobalCheckerV2::check(const std::string& filepath) {
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

  clang::ast_matchers::MatchFinder Finder;
  registerMatchers(Finder);

  GlobalV2FrontendActionFactory factory(Finder);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

void GlobalCheckerV2::registerMatchers(clang::ast_matchers::MatchFinder& Finder) {
  using namespace clang::ast_matchers;

  clang::ast_matchers::DeclarationMatcher globalVarMatcher =
      varDecl(hasGlobalStorage()).bind("var");

  Finder.addMatcher(globalVarMatcher, this);
}

void GlobalCheckerV2::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  if (const clang::VarDecl* VD = Result.Nodes.getNodeAs<clang::VarDecl>("var")) {
    if (checkIsInSystemHeader(VD, *Result.Context)) {
      return;
    }
    if (isExternWithoutInit(VD)) {
      return;
    }
    reportGlobalVariable(VD, Result);
  }
}

void GlobalCheckerV2::reportGlobalVariable(
    const clang::VarDecl* VD, const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  if (!VD) {
    return;
  }

  std::string name = VD->getName().str();
  if (name.empty()) {
    return;
  }

  clang::QualType type = VD->getType();
  std::string type_str = type.getAsString();

  clang::SourceLocation loc = VD->getLocation();
  const clang::SourceManager& SM = Result.Context->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

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

bool GlobalCheckerV2::checkIsInSystemHeader(const clang::VarDecl* VD,
                                            const clang::ASTContext& Context) const {
  if (!VD) {
    return false;
  }

  clang::SourceLocation loc = VD->getLocation();
  const clang::SourceManager& SM = Context.getSourceManager();

  clang::FileID fileID = SM.getFileID(loc);
  if (fileID.isInvalid()) {
    return true;
  }

  auto fileEntryRef = SM.getFileEntryRefForID(fileID);
  if (!fileEntryRef) {
    return true;
  }

  std::string filename = fileEntryRef->getName().str();
  return filename.find("/usr/include/") != std::string::npos ||
         filename.find("/usr/lib/") != std::string::npos ||
         filename.find("/usr/local/include/") != std::string::npos ||
         filename.find("/Library/Developer/") != std::string::npos || filename.empty();
}

bool GlobalCheckerV2::isExternWithoutInit(const clang::VarDecl* VD) const {
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

} // namespace lint
} // namespace codelint
