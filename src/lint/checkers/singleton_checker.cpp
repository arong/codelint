#include "lint/checkers/singleton_checker.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

namespace codelint {
namespace lint {
SingletonChecker::SingletonChecker(const std::optional<GitScope>& scope) : scope_(scope) {
}

class SingletonASTConsumer : public clang::ASTConsumer {
public:
  explicit SingletonASTConsumer(SingletonChecker* checker) : checker_(checker) {
  }

  void HandleTranslationUnit(clang::ASTContext& Context) override {
    checker_->runOnAST(&Context);
  }

private:
  SingletonChecker* checker_;
};

class SingletonFrontendAction : public clang::ASTFrontendAction {
public:
  explicit SingletonFrontendAction(SingletonChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                        llvm::StringRef InFile) override {
    return std::make_unique<SingletonASTConsumer>(checker_);
  }

private:
  SingletonChecker* checker_;
};

class SingletonFrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
  explicit SingletonFrontendActionFactory(SingletonChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<SingletonFrontendAction>(checker_);
  }

private:
  SingletonChecker* checker_;
};

LintResult SingletonChecker::check(const std::string& filepath) {
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

  SingletonFrontendActionFactory factory(this);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

void SingletonChecker::runOnAST(clang::ASTContext* Context) {
  Context_ = Context;
  TraverseDecl(Context->getTranslationUnitDecl());
}

bool SingletonChecker::VisitFunctionDecl(clang::FunctionDecl* FD) {
  if (!FD || !Context_) {
    return true;
  }

  if (isInSystemHeader(FD)) {
    return true;
  }

  if (!returnsReference(FD)) {
    return true;
  }

  std::string staticVarName;
  if (hasMeyersSingletonPattern(FD, staticVarName)) {
    reportSingletonPattern(FD, staticVarName);
  }

  return true;
}

bool SingletonChecker::returnsReference(clang::FunctionDecl* FD) const {
  if (!FD) {
    return false;
  }

  clang::QualType returnType = FD->getReturnType();
  return returnType->isLValueReferenceType();
}

bool SingletonChecker::hasMeyersSingletonPattern(clang::FunctionDecl* FD,
                                                 std::string& staticVarName) const {
  if (!FD || !FD->hasBody()) {
    return false;
  }

  clang::Stmt* body = FD->getBody();

  clang::VarDecl* staticLocalVar = nullptr;

  for (auto* child : body->children()) {
    if (auto* declStmt = llvm::dyn_cast<clang::DeclStmt>(child)) {
      for (auto* decl : declStmt->decls()) {
        if (auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl)) {
          if (varDecl->isStaticLocal()) {
            staticLocalVar = varDecl;
            break;
          }
        }
      }
    }
    if (staticLocalVar)
      break;
  }

  if (!staticLocalVar) {
    return false;
  }

  clang::VarDecl* returnedStaticVar = nullptr;

  for (auto* child : body->children()) {
    if (auto* returnStmt = llvm::dyn_cast<clang::ReturnStmt>(child)) {
      clang::Expr* retValue = returnStmt->getRetValue();
      if (retValue) {
        retValue = retValue->IgnoreParenImpCasts();
        if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(retValue)) {
          if (auto* varDecl = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            if (varDecl == staticLocalVar) {
              returnedStaticVar = varDecl;
              break;
            }
          }
        }
      }
    }
  }

  if (returnedStaticVar) {
    staticVarName = returnedStaticVar->getName().str();
    return true;
  }

  return false;
}

bool SingletonChecker::isInSystemHeader(clang::Decl* D) const {
  if (!D || !Context_) {
    return false;
  }

  clang::SourceLocation loc = D->getLocation();
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
  return filename.find("/usr/include/") == 0 || filename.find("/usr/lib/") == 0 ||
         filename.find("/usr/local/include/") == 0 || filename.empty();
}

void SingletonChecker::reportSingletonPattern(clang::FunctionDecl* FD,
                                              const std::string& staticVarName) {
  if (!FD || !Context_) {
    return;
  }

  std::string funcName = FD->getNameAsString();
  if (funcName.empty()) {
    return;
  }

  clang::QualType returnType = FD->getReturnType();
  std::string return_type_str = returnType.getAsString();

  // Get class name if method is part of a class
  std::string class_name;
  if (auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(FD->getParent())) {
    class_name = recordDecl->getNameAsString();
  }

  clang::SourceLocation loc = FD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();

  std::string file = SM.getFilename(loc).str();
  int line = static_cast<int>(SM.getExpansionLineNumber(loc));
  int column = static_cast<int>(SM.getExpansionColumnNumber(loc));

  LintIssue issue;
  issue.type = CheckType::SINGLETON_PATTERN;
  issue.severity = Severity::INFO;
  issue.checker_name = "singleton";
  issue.name = funcName;
  issue.type_str = return_type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  
  // Build description and suggestion with class name if available
  if (!class_name.empty()) {
    issue.description = "Singleton pattern detected in " + class_name + "::" + funcName;
    issue.suggestion = class_name + "::" + funcName + "()";
  } else {
    issue.description = "Singleton pattern detected";
    issue.suggestion = funcName + "()";
  }
  issue.fixable = false;

  Reporter_.add_issue(issue);
}

} // namespace lint
} // namespace codelint
