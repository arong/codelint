#include "lint/checkers/init_checker_v2.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

using namespace clang::ast_matchers;

namespace codelint {
namespace lint {

InitCheckerV2::InitCheckerV2(const std::optional<GitScope>& scope) : scope_(scope) {
}

void InitCheckerV2::registerMatchers(MatchFinder& Finder) {
  Finder.addMatcher(
      varDecl(unless(hasInitializer(anything())), unless(isImplicit()), unless(parmVarDecl()))
          .bind("uninit"),
      this);

  Finder.addMatcher(varDecl(hasInitializer(expr()), unless(hasInitializer(initListExpr())),
                            unless(isImplicit()), unless(parmVarDecl()))
                        .bind("equalsSyntax"),
                    this);

  Finder.addMatcher(varDecl(hasInitializer(integerLiteral().bind("literal")), unless(isImplicit()),
                            unless(parmVarDecl()))
                        .bind("unsignedCandidate"),
                    this);
}

void InitCheckerV2::run(const MatchFinder::MatchResult& Result) {
  Context_ = Result.Context;

  if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("uninit")) {
    if (!shouldSkipVarDecl(VD, Result.Context) && !isInSystemHeader(VD, Result.Context)) {
      handleUninitialized(VD, Result.Context);
    }
  }

  if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("equalsSyntax")) {
    if (!shouldSkipVarDecl(VD, Result.Context) && !isInSystemHeader(VD, Result.Context)) {
      handleEqualsSyntax(VD, Result.Context);
    }
  }

  if (const auto* VD = Result.Nodes.getNodeAs<clang::VarDecl>("unsignedCandidate")) {
    if (!shouldSkipVarDecl(VD, Result.Context) && !isInSystemHeader(VD, Result.Context)) {
      if (needsUnsignedSuffix(VD, Result.Context)) {
        handleUnsignedSuffix(VD, Result.Context);
      }
    }
  }
}

bool InitCheckerV2::shouldSkipVarDecl(const clang::VarDecl* VD, clang::ASTContext* Ctx) const {
  if (!VD)
    return true;

  if (shouldSkipForLoop(VD, Ctx))
    return true;
  if (shouldSkipAuto(VD))
    return true;
  if (shouldSkipLambda(VD, Ctx))
    return true;

  return false;
}

bool InitCheckerV2::shouldSkipForLoop(const clang::VarDecl* VD, clang::ASTContext* Ctx) const {
  if (!VD || !Ctx)
    return false;

  auto parents = Ctx->getParents(*VD);
  for (const auto& parent : parents) {
    if (parent.get<clang::ForStmt>()) {
      return true;
    }
  }
  return false;
}

bool InitCheckerV2::shouldSkipAuto(const clang::VarDecl* VD) const {
  if (!VD)
    return false;

  clang::QualType type = VD->getType();
  if (type->isUndeducedAutoType()) {
    return true;
  }

  std::string typeStr = type.getAsString();
  return typeStr.find("auto") != std::string::npos;
}

bool InitCheckerV2::shouldSkipLambda(const clang::VarDecl* VD, clang::ASTContext* Ctx) const {
  if (!VD || !Ctx)
    return false;

  if (auto* init = VD->getInit()) {
    if (clang::isa<clang::LambdaExpr>(init->IgnoreImplicit())) {
      return true;
    }
  }
  return false;
}

bool InitCheckerV2::isInSystemHeader(const clang::VarDecl* VD, clang::ASTContext* Ctx) const {
  if (!VD || !Ctx)
    return true;

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Ctx->getSourceManager();

  clang::FileID fileID = SM.getFileID(loc);
  if (fileID.isInvalid())
    return true;

  auto fileEntryRef = SM.getFileEntryRefForID(fileID);
  if (!fileEntryRef)
    return true;

  std::string filename = fileEntryRef->getName().str();
  return filename.find("/usr/include/") != std::string::npos ||
         filename.find("/usr/lib/") != std::string::npos ||
         filename.find("/usr/local/include/") != std::string::npos ||
         filename.find("/Library/Developer/") != std::string::npos ||
         filename.find("/opt/homebrew/") != std::string::npos || filename.empty();
}

bool InitCheckerV2::needsUnsignedSuffix(const clang::VarDecl* VD, clang::ASTContext* Ctx) const {
  if (!VD || !Ctx)
    return false;

  clang::QualType type = VD->getType();
  if (!type->isUnsignedIntegerType()) {
    return false;
  }

  if (auto* init = VD->getInit()) {
    if (auto* literal = clang::dyn_cast<clang::IntegerLiteral>(init->IgnoreImplicit())) {
      return !hasSuffixAlready(literal, Ctx);
    }
  }

  return false;
}

bool InitCheckerV2::hasSuffixAlready(const clang::IntegerLiteral* literal,
                                     clang::ASTContext* Ctx) const {
  if (!literal || !Ctx)
    return true;

  clang::SourceManager& SM = Ctx->getSourceManager();
  clang::SourceRange range = literal->getSourceRange();

  clang::StringRef text = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range),
                                                      SM, Ctx->getLangOpts());

  std::string str = text.str();
  return str.find('U') != std::string::npos || str.find('u') != std::string::npos;
}

void InitCheckerV2::handleUninitialized(const clang::VarDecl* VD, clang::ASTContext* Ctx) {
  if (!VD || !Ctx)
    return;

  std::string name = VD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = VD->getType();
  std::string type_str = type.getAsString();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Ctx->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

  LintIssue issue;
  issue.type = CheckType::UNINITIALIZED_VAR;
  issue.severity = Severity::WARNING;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Variable is not explicitly initialized";
  issue.suggestion = type_str + " " + name + "{}";
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

void InitCheckerV2::handleEqualsSyntax(const clang::VarDecl* VD, clang::ASTContext* Ctx) {
  if (!VD || !Ctx)
    return;

  std::string name = VD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = VD->getType();
  std::string type_str = type.getAsString();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Ctx->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

  LintIssue issue;
  issue.type = CheckType::INIT_SYNTAX;
  issue.severity = Severity::INFO;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Variable initialized with '=' should use '{}' syntax";
  issue.suggestion = type_str + " " + name + "{...}";
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

void InitCheckerV2::handleUnsignedSuffix(const clang::VarDecl* VD, clang::ASTContext* Ctx) {
  if (!VD || !Ctx)
    return;

  std::string name = VD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = VD->getType();
  std::string type_str = type.getAsString();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Ctx->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

  LintIssue issue;
  issue.type = CheckType::INIT_UNSIGNED_SUFFIX;
  issue.severity = Severity::HINT;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Unsigned integer should have 'U' or 'UL' suffix";
  issue.suggestion = type_str + " " + name + "{...U}";
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

LintResult InitCheckerV2::check(const std::string& filepath) {
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
