#include "lint/checkers/init_checker.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace codelint::lint {

class InitASTConsumer : public clang::ASTConsumer {
public:
  explicit InitASTConsumer(InitChecker* checker) : checker_(checker) {
  }

  void HandleTranslationUnit(clang::ASTContext& Context) override {
    checker_->runOnAST(&Context);
  }

private:
  InitChecker* checker_;
};

class InitFrontendAction : public clang::ASTFrontendAction {
public:
  explicit InitFrontendAction(InitChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                        llvm::StringRef InFile) override {
    return std::make_unique<InitASTConsumer>(checker_);
  }

private:
  InitChecker* checker_;
};

class InitFrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
  explicit InitFrontendActionFactory(InitChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<InitFrontendAction>(checker_);
  }

private:
  InitChecker* checker_;
};
InitChecker::InitChecker(const std::optional<GitScope>& scope) : scope_(scope) {
}

LintResult InitChecker::check(const std::string& filepath) {
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

  InitFrontendActionFactory factory(this);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

void InitChecker::runOnAST(clang::ASTContext* Context) {
  Context_ = Context;
  clang::TranslationUnitDecl* TU = Context->getTranslationUnitDecl();

  for (clang::Decl* D : TU->decls()) {
    if (clang::VarDecl* VD = clang::dyn_cast<clang::VarDecl>(D)) {
      VisitVarDecl(VD);
    }
    if (clang::FunctionDecl* FD = clang::dyn_cast<clang::FunctionDecl>(D)) {
      TraverseDecl(FD);
    }
    if (clang::RecordDecl* RD = clang::dyn_cast<clang::RecordDecl>(D)) {
      processRecordDecl(RD);
    }
  }
}

void InitChecker::processRecordDecl(clang::RecordDecl* RD) {
  for (clang::Decl* D : RD->decls()) {
    if (clang::FieldDecl* FD = clang::dyn_cast<clang::FieldDecl>(D)) {
      VisitFieldDecl(FD);
    }
    if (clang::RecordDecl* nestedRD = clang::dyn_cast<clang::RecordDecl>(D)) {
      processRecordDecl(nestedRD);
    }
  }
}

bool InitChecker::VisitVarDecl(clang::VarDecl* VD) {
  if (!VD || !Context_) {
    return true;
  }

  if (clang::dyn_cast<clang::ParmVarDecl>(VD)) {
    return true;
  }

  if (isInSystemHeader(VD) || !isMainFile(VD)) {
    return true;
  }

  if (shouldSkipAutoDeclaration(VD) || shouldSkipForLoopVariable(VD) || shouldSkipUnionMember(VD) ||
      shouldSkipEnumClassWithoutZero(VD) || shouldSkipExternDeclaration(VD) ||
      shouldSkipExceptionVariable(VD) || shouldSkipInitializerListConstructor(VD) ||
      shouldSkipLambdaParameter(VD)) {
    return true;
  }

  clang::Expr* init = VD->getInit();

  if (!init) {
    checkUninitialized(VD);
  } else {
    bool is_brace_init = clang::isa<clang::InitListExpr>(init);

    clang::Expr* ignoredInit = init->IgnoreImplicit();
    clang::CXXConstructExpr* constructExpr = clang::dyn_cast<clang::CXXConstructExpr>(ignoredInit);

    bool is_implicit_construct =
        constructExpr && !clang::isa<clang::InitListExpr>(init) && constructExpr->getNumArgs() == 0;

    if (is_implicit_construct) {
      checkUninitialized(VD);
    } else if (!is_brace_init && VD->getInitStyle() == clang::VarDecl::CInit) {
      checkEqualsInit(VD);
    } else if (constructExpr && constructExpr->getNumArgs() > 0 && !is_brace_init) {
      checkEqualsInit(VD);
    }
    checkUnsignedSuffix(VD);
  }

  return true;
}

bool InitChecker::VisitFieldDecl(clang::FieldDecl* FD) {
  if (!FD || !Context_) {
    return true;
  }

  if (isInSystemHeader(FD) || !isMainFile(FD)) {
    return true;
  }

  clang::DeclContext* DC = FD->getDeclContext();
  if (clang::RecordDecl* RD = clang::dyn_cast<clang::RecordDecl>(DC)) {
    if (RD->isUnion()) {
      return true;
    }
  }

  clang::Expr* init = FD->getInClassInitializer();
  if (init) {
    return true;
  }

  checkFieldUninitialized(FD);

  return true;
}

void InitChecker::checkUninitialized(clang::VarDecl* VD) {
  std::string name = VD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = VD->getType();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();
  std::string file = SM.getFilename(loc).str();
  int line = SM.getExpansionLineNumber(loc);
  int column = SM.getExpansionColumnNumber(loc);

  clang::SourceRange typeRange(VD->getBeginLoc(), VD->getLocation());
  std::string type_str =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(typeRange), SM,
                                  Context_->getLangOpts())
          .str();

  size_t name_pos = type_str.find(name);
  if (name_pos != std::string::npos) {
    type_str = type_str.substr(0, name_pos);
    while (!type_str.empty() && (type_str.back() == ' ' || type_str.back() == '\t')) {
      type_str.pop_back();
    }
  }

  std::string full_decl =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(VD->getSourceRange()), SM,
                                  Context_->getLangOpts())
          .str();

  size_t semi_pos = full_decl.find(';');
  if (semi_pos != std::string::npos) {
    full_decl = full_decl.substr(0, semi_pos);
  }

  std::string suggestion;
  if (type->isArrayType()) {
    suggestion = full_decl + "{};";
  } else {
    suggestion = type_str + " " + name + "{};";
  }

  LintIssue issue;
  issue.type = CheckType::INIT_UNINITIALIZED;
  issue.severity = Severity::WARNING;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Variable is not explicitly initialized";
  issue.suggestion = suggestion;
  issue.fixable = true;

  Reporter_.add_issue(issue);
}

void InitChecker::checkEqualsInit(clang::VarDecl* VD) {
  std::string name = VD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = VD->getType();

  clang::SourceLocation loc = VD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();
  std::string file = SM.getFilename(loc).str();
  int line = SM.getExpansionLineNumber(loc);
  int column = SM.getExpansionColumnNumber(loc);

  clang::SourceRange typeRange(VD->getBeginLoc(), VD->getLocation());
  std::string type_str =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(typeRange), SM,
                                  Context_->getLangOpts())
          .str();

  size_t name_pos = type_str.find(name);
  if (name_pos != std::string::npos) {
    type_str = type_str.substr(0, name_pos);
    while (!type_str.empty() && (type_str.back() == ' ' || type_str.back() == '\t')) {
      type_str.pop_back();
    }
  }

  clang::Expr* init = VD->getInit();
  std::string init_value;
  if (init) {
    clang::Expr* ignoredInit = init->IgnoreImplicit();
    clang::CXXConstructExpr* constructExpr = clang::dyn_cast<clang::CXXConstructExpr>(ignoredInit);

    if (constructExpr && constructExpr->getNumArgs() > 0) {
      std::stringstream args_ss;
      bool first = true;
      for (unsigned i = 0; i < constructExpr->getNumArgs(); ++i) {
        clang::Expr* arg = constructExpr->getArg(i);
        if (arg && !clang::isa<clang::CXXDefaultArgExpr>(arg)) {
          if (!first)
            args_ss << ", ";
          first = false;
          args_ss << clang::Lexer::getSourceText(
                         clang::CharSourceRange::getTokenRange(arg->getSourceRange()), SM,
                         Context_->getLangOpts())
                         .str();
        }
      }
      init_value = args_ss.str();
    } else {
      init_value =
          clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(init->getSourceRange()),
                                      SM, Context_->getLangOpts())
              .str();
    }
  }

  bool is_unsigned = isUnsignedType(type);
  std::string suggestion;
  if (is_unsigned && hasDigitValue(init_value) && !hasUnsignedSuffix(init_value)) {
    suggestion = type_str + " " + name + "{" + init_value + "U};";
  } else {
    suggestion = type_str + " " + name + "{" + init_value + "};";
  }

  LintIssue issue;
  issue.type = CheckType::INIT_EQUALS_SYNTAX;
  issue.severity = Severity::INFO;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Variable initialized with '=' should use '{}' syntax";
  issue.suggestion = suggestion;
  issue.fixable = true;

  Reporter_.add_issue(issue);
}

void InitChecker::checkFieldUninitialized(clang::FieldDecl* FD) {
  std::string name = FD->getName().str();
  if (name.empty())
    return;

  clang::QualType type = FD->getType();

  clang::SourceLocation loc = FD->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();
  std::string file = SM.getFilename(loc).str();
  int line = SM.getExpansionLineNumber(loc);
  int column = SM.getExpansionColumnNumber(loc);

  clang::SourceRange typeRange(FD->getBeginLoc(), FD->getLocation());
  std::string type_str =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(typeRange), SM,
                                  Context_->getLangOpts())
          .str();

  size_t name_pos = type_str.find(name);
  if (name_pos != std::string::npos) {
    type_str = type_str.substr(0, name_pos);
    while (!type_str.empty() && (type_str.back() == ' ' || type_str.back() == '\t')) {
      type_str.pop_back();
    }
  }

  LintIssue issue;
  issue.type = CheckType::INIT_UNINITIALIZED;
  issue.severity = Severity::WARNING;
  issue.checker_name = "init";
  issue.name = name;
  issue.type_str = type_str;
  issue.file = file;
  issue.line = line;
  issue.column = column;
  issue.description = "Field is not explicitly initialized";
  issue.suggestion = type_str + " " + name + "{};";
  issue.fixable = true;

  Reporter_.add_issue(issue);
}

void InitChecker::checkUnsignedSuffix(clang::VarDecl* VD) {
  if (!isUnsignedType(VD->getType()))
    return;

  clang::Expr* init = VD->getInit();
  if (!init)
    return;

  std::string init_value =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(init->getSourceRange()),
                                  Context_->getSourceManager(), Context_->getLangOpts())
          .str();

  if (hasDigitValue(init_value) && !hasUnsignedSuffix(init_value)) {
    std::string name = VD->getName().str();

    clang::SourceLocation loc = VD->getLocation();
    clang::SourceManager& SM = Context_->getSourceManager();

    clang::SourceRange typeRange(VD->getBeginLoc(), VD->getLocation());
    std::string type_str =
        clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(typeRange), SM,
                                    Context_->getLangOpts())
            .str();

    size_t name_pos = type_str.find(name);
    if (name_pos != std::string::npos) {
      type_str = type_str.substr(0, name_pos);
      while (!type_str.empty() && (type_str.back() == ' ' || type_str.back() == '\t')) {
        type_str.pop_back();
      }
    }

    std::string fixed_value = init_value + "U";
    std::string suggestion = type_str + " " + name + "{" + fixed_value + "};";

    LintIssue issue;
    issue.type = CheckType::INIT_UNSIGNED_SUFFIX;
    issue.severity = Severity::HINT;
    issue.checker_name = "init";
    issue.name = name;
    issue.type_str = type_str;
    issue.file = SM.getFilename(loc).str();
    issue.line = SM.getExpansionLineNumber(loc);
    issue.column = SM.getExpansionColumnNumber(loc);
    issue.description = "Unsigned integer should have 'U' or 'UL' suffix";
    issue.suggestion = suggestion;
    issue.fixable = true;

    Reporter_.add_issue(issue);
  }
}

bool InitChecker::shouldSkipAutoDeclaration(clang::VarDecl* VD) {
  clang::QualType type = VD->getType();
  if (type->isUndeducedAutoType()) {
    return true;
  }

  clang::SourceRange range = VD->getSourceRange();
  clang::SourceManager& SM = Context_->getSourceManager();
  clang::LangOptions langOpts;
  llvm::StringRef text =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range), SM, langOpts);

  if (text.starts_with("auto ")) {
    return true;
  }

  return false;
}

bool InitChecker::shouldSkipForLoopVariable(clang::VarDecl* VD) {
  clang::DynTypedNodeList parents = Context_->getParents(*VD);
  for (const auto& parent : parents) {
    if (parent.get<clang::ForStmt>() || parent.get<clang::CXXForRangeStmt>()) {
      return true;
    }
    const clang::DeclStmt* declStmt = parent.get<clang::DeclStmt>();
    if (declStmt) {
      clang::DynTypedNodeList grandParents = Context_->getParents(*declStmt);
      for (const auto& grandParent : grandParents) {
        if (grandParent.get<clang::ForStmt>() || grandParent.get<clang::CXXForRangeStmt>()) {
          return true;
        }
      }
    }
  }
  return false;
}

bool InitChecker::shouldSkipUnionMember(clang::VarDecl* VD) {
  clang::DeclContext* DC = VD->getDeclContext();
  if (clang::RecordDecl* RD = clang::dyn_cast<clang::RecordDecl>(DC)) {
    return RD->isUnion();
  }
  return false;
}

bool InitChecker::shouldSkipEnumClassWithoutZero(clang::VarDecl* VD) {
  clang::QualType type = VD->getType();
  const clang::EnumType* ET = type->getAs<clang::EnumType>();
  if (!ET)
    return false;

  clang::EnumDecl* ED = ET->getDecl();
  if (!ED || !ED->isScoped())
    return false;

  for (clang::EnumConstantDecl* ECD : ED->enumerators()) {
    if (ECD->getInitVal() == 0) {
      return false;
    }
  }
  return true;
}

bool InitChecker::shouldSkipExternDeclaration(clang::VarDecl* VD) {
  return VD->getStorageClass() == clang::SC_Extern;
}

bool InitChecker::shouldSkipExceptionVariable(clang::VarDecl* VD) {
  return VD->isExceptionVariable();
}

bool InitChecker::shouldSkipInitializerListConstructor(clang::VarDecl* VD) {
  clang::Expr* init = VD->getInit();
  if (!init)
    return false;

  clang::CXXConstructExpr* constructExpr = clang::dyn_cast<clang::CXXConstructExpr>(init);
  if (!constructExpr)
    return false;

  clang::CXXConstructorDecl* constructor = constructExpr->getConstructor();
  if (!constructor)
    return false;

  for (unsigned i = 0; i < constructor->getNumParams(); ++i) {
    clang::ParmVarDecl* param = constructor->getParamDecl(i);
    clang::QualType paramType = param->getType();
    std::string typeStr = paramType.getAsString();
    if (typeStr.find("initializer_list") != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool InitChecker::isInSystemHeader(clang::Decl* D) const {
  if (!D || !Context_)
    return false;

  clang::SourceLocation loc = D->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();

  clang::FileID fileID = SM.getFileID(loc);
  if (fileID.isInvalid())
    return true;

  auto fileEntryRef = SM.getFileEntryRefForID(fileID);
  if (!fileEntryRef)
    return true;

  std::string filename = fileEntryRef->getName().str();
  return filename.find("/usr/include/") == 0 || filename.find("/usr/lib/") == 0 ||
         filename.find("/usr/local/include/") == 0 || filename.empty();
}

bool InitChecker::isMainFile(clang::Decl* D) const {
  if (!D || !Context_)
    return false;

  clang::SourceLocation loc = D->getLocation();
  clang::SourceManager& SM = Context_->getSourceManager();

  clang::FileID mainFileID = SM.getMainFileID();
  clang::FileID declFileID = SM.getFileID(loc);

  return declFileID == mainFileID;
}

bool InitChecker::isUnsignedType(clang::QualType type) const {
  std::string type_str = type.getAsString();
  return type_str.find("unsigned") != std::string::npos;
}

bool InitChecker::hasDigitValue(const std::string& value) const {
  return std::any_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool InitChecker::hasUnsignedSuffix(const std::string& value) const {
  return value.find('U') != std::string::npos || value.find('u') != std::string::npos;
}

bool InitChecker::shouldSkipLambdaParameter(clang::VarDecl* VD) {
  clang::ParmVarDecl* PVD = clang::dyn_cast<clang::ParmVarDecl>(VD);
  if (!PVD)
    return false;

  clang::DynTypedNodeList parents = Context_->getParents(*PVD);
  for (const auto& parent : parents) {
    if (parent.get<clang::LambdaExpr>()) {
      return true;
    }
    const clang::FunctionDecl* FD = parent.get<clang::FunctionDecl>();
    if (FD) {
      clang::DynTypedNodeList grandParents = Context_->getParents(*FD);
      for (const auto& grandParent : grandParents) {
        if (grandParent.get<clang::LambdaExpr>()) {
          return true;
        }
      }
    }
  }
  return false;
}

bool InitChecker::shouldSkipCatchVariableCopy(clang::VarDecl* VD) {
  clang::Expr* init = VD->getInit();
  if (!init)
    return false;

  clang::Expr* ignored = init->IgnoreImplicit();
  clang::CXXConstructExpr* constructExpr = clang::dyn_cast<clang::CXXConstructExpr>(ignored);

  if (constructExpr && constructExpr->getNumArgs() == 1) {
    clang::Expr* arg = constructExpr->getArg(0)->IgnoreImplicit();
    clang::DeclRefExpr* DRE = clang::dyn_cast<clang::DeclRefExpr>(arg);
    if (DRE) {
      clang::VarDecl* referencedVar = clang::dyn_cast<clang::VarDecl>(DRE->getDecl());
      if (referencedVar && referencedVar->isExceptionVariable()) {
        return true;
      }
    }
  }

  clang::DeclRefExpr* DRE = clang::dyn_cast<clang::DeclRefExpr>(ignored);
  if (!DRE)
    return false;

  clang::ValueDecl* VD_ref = DRE->getDecl();
  if (!VD_ref)
    return false;

  clang::VarDecl* referencedVar = clang::dyn_cast<clang::VarDecl>(VD_ref);
  if (!referencedVar)
    return false;

  return referencedVar->isExceptionVariable();
}

bool InitChecker::apply_fixes(const std::string& filepath, const std::vector<LintIssue>& issues,
                              std::string& modified_content) {
  std::vector<std::string> lines;
  std::stringstream ss(modified_content);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  std::vector<LintIssue> sorted_issues = issues;
  std::sort(sorted_issues.begin(), sorted_issues.end(),
            [](const LintIssue& a, const LintIssue& b) { return a.line > b.line; });

  for (const auto& issue : sorted_issues) {
    if (issue.checker_name != "init")
      continue;
    if (issue.line <= 0 || issue.line > (int)lines.size())
      continue;

    int idx = issue.line - 1;

    if (issue.type == CheckType::INIT_UNINITIALIZED) {
      size_t pos = lines[idx].find(issue.name);
      while (pos != std::string::npos) {
        size_t name_end = pos + issue.name.length();
        while (name_end < lines[idx].length() &&
               (lines[idx][name_end] == ' ' || lines[idx][name_end] == '\t')) {
          name_end++;
        }

        if (name_end < lines[idx].length()) {
          char next_char = lines[idx][name_end];
          if (next_char == ';') {
            lines[idx].insert(name_end, "{}");
            break;
          } else if (next_char == ',') {
            lines[idx].insert(name_end, "{}");
            break;
          } else if (next_char == '[') {
            size_t scan_pos = name_end;
            while (scan_pos < lines[idx].length() && lines[idx][scan_pos] == '[') {
              size_t bracket_end = lines[idx].find(']', scan_pos);
              if (bracket_end == std::string::npos)
                break;
              scan_pos = bracket_end + 1;
              while (scan_pos < lines[idx].length() &&
                     (lines[idx][scan_pos] == ' ' || lines[idx][scan_pos] == '\t')) {
                scan_pos++;
              }
            }
            if (scan_pos < lines[idx].length() &&
                (lines[idx][scan_pos] == ';' || lines[idx][scan_pos] == ',')) {
              lines[idx].insert(scan_pos, "{}");
              break;
            }
          }
        }
        pos = lines[idx].find(issue.name, pos + 1);
      }
    } else if (issue.type == CheckType::INIT_EQUALS_SYNTAX) {
      size_t pos = lines[idx].find("=");
      size_t paren_pos = lines[idx].find("(");

      size_t replace_pos = (pos != std::string::npos) ? pos : paren_pos;

      size_t comment_pos = lines[idx].find("//");
      if (comment_pos == std::string::npos || comment_pos > replace_pos) {
        if (!issue.suggestion.empty()) {
          size_t indent_end = lines[idx].find_first_not_of(" \t");
          std::string indent =
              (indent_end != std::string::npos) ? lines[idx].substr(0, indent_end) : "";
          lines[idx] = indent + issue.suggestion;
        }
      }
    }
  }

  std::stringstream output;
  for (const auto& l : lines) {
    output << l << "\n";
  }
  modified_content = output.str();
  return true;
}

} // namespace codelint::lint
