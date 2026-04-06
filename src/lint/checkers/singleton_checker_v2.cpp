#include "lint/checkers/singleton_checker_v2.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

using namespace clang::ast_matchers;

namespace codelint {
namespace lint {

SingletonCheckerV2::SingletonCheckerV2(const std::optional<GitScope>& scope) : scope_(scope) {
}

void SingletonCheckerV2::registerMatchers(MatchFinder& Finder) {
  auto hasStaticLocalReturn =
      functionDecl(returns(hasCanonicalType(referenceType())),
                   hasBody(compoundStmt(
                       has(declStmt(has(varDecl(hasStaticStorageDuration()).bind("staticVar")))),
                       hasReturnStmt(has(declRefExpr(to(varDecl(hasStaticStorageDuration()))))))))
          .bind("singletonFunction");

  Finder.addMatcher(hasStaticLocalReturn, this);
}

void SingletonCheckerV2::run(const MatchFinder::MatchResult& Result) {
  if (const auto* FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("singletonFunction")) {
    if (!isInSystemHeader(FD, Result.Context)) {
      if (const auto* staticVar = Result.Nodes.getNodeAs<clang::VarDecl>("staticVar")) {
        std::string staticVarName = staticVar->getName().str();
        reportSingletonPattern(FD, staticVarName, Result.Context);
      }
    }
  }
}

bool SingletonCheckerV2::isInSystemHeader(const clang::Decl* D, clang::ASTContext* Context) const {
  if (!D || !Context) {
    return true;
  }

  clang::SourceLocation loc = D->getLocation();
  clang::SourceManager& SM = Context->getSourceManager();

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
         filename.find("/Library/Developer/") != std::string::npos ||
         filename.find("/opt/homebrew/") != std::string::npos || filename.empty();
}

void SingletonCheckerV2::reportSingletonPattern(const clang::FunctionDecl* FD,
                                                const std::string& staticVarName,
                                                clang::ASTContext* Context) {
  if (!FD || !Context) {
    return;
  }

  std::string className;
  std::string funcName = FD->getName().str();

  if (const auto* MD = clang::dyn_cast<clang::CXXMethodDecl>(FD)) {
    className = MD->getParent()->getName().str();
  }

  clang::QualType returnType = FD->getReturnType();
  std::string type_str = returnType.getAsString();

  clang::SourceLocation loc = FD->getLocation();
  clang::SourceManager& SM = Context->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

  LintIssue issue;
  issue.type = CheckType::SINGLETON_PATTERN;
  issue.severity = Severity::INFO;
  issue.checker_name = "singleton";
  issue.name = funcName;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;

  std::string fullName = className.empty() ? funcName : className + "::" + funcName;
  issue.description = "Singleton pattern detected in " + fullName;
  issue.suggestion = fullName + "()";
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

LintResult SingletonCheckerV2::check(const std::string& filepath) {
  Result_.issues.clear();
  Result_.error_count = 0;
  Result_.warning_count = 0;
  Result_.info_count = 0;
  Result_.hint_count = 0;
  Reporter_.clear();

  Finder_ = std::make_unique<MatchFinder>();
  registerMatchers(*Finder_);

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

  class MatchFinderAction : public clang::ASTFrontendAction {
  public:
    MatchFinderAction(MatchFinder& Finder) : Finder_(Finder) {
    }

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                          llvm::StringRef InFile) override {
      return Finder_.newASTConsumer();
    }

  private:
    MatchFinder& Finder_;
  };

  class MatchFinderActionFactory : public clang::tooling::FrontendActionFactory {
  public:
    MatchFinderActionFactory(MatchFinder& Finder) : Finder_(Finder) {
    }

    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<MatchFinderAction>(Finder_);
    }

  private:
    MatchFinder& Finder_;
  };

  std::vector<std::string> sources = {filepath};
  clang::tooling::ClangTool tool(*compilations, sources);

  MatchFinderActionFactory factory(*Finder_);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

} // namespace lint
} // namespace codelint
