#include "lint/checkers/const_checker.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

#include <sstream>

namespace codelint {
namespace lint {
ConstChecker::ConstChecker(const std::optional<GitScope>& scope) : scope_(scope) {
}

class ConstASTConsumer : public clang::ASTConsumer {
public:
  explicit ConstASTConsumer(ConstChecker* checker) : checker_(checker) {
  }

  void HandleTranslationUnit(clang::ASTContext& Context) override {
    checker_->runOnAST(&Context);
  }

private:
  ConstChecker* checker_;
};

class ConstFrontendAction : public clang::ASTFrontendAction {
public:
  explicit ConstFrontendAction(ConstChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                        llvm::StringRef InFile) override {
    return std::make_unique<ConstASTConsumer>(checker_);
  }

private:
  ConstChecker* checker_;
};

class ConstFrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
  explicit ConstFrontendActionFactory(ConstChecker* checker) : checker_(checker) {
  }

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<ConstFrontendAction>(checker_);
  }

private:
  ConstChecker* checker_;
};

LintResult ConstChecker::check(const std::string& filepath) {

  Result_.issues.clear();
  Result_.error_count = 0;
  Result_.warning_count = 0;
  Result_.info_count = 0;
  Result_.hint_count = 0;
  Reporter_.clear();
  variables_.clear();
  modified_vars_.clear();

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

  ConstFrontendActionFactory factory(this);
  tool.run(&factory);

  for (const auto& issue : Reporter_.issues()) {
    Result_.add_issue(issue);
  }

  return Result_;
}

void ConstChecker::runOnAST(clang::ASTContext* Context) {
  Context_ = Context;

  clang::TranslationUnitDecl* TU = Context->getTranslationUnitDecl();

  TraverseDecl(TU);

  analyzeAndReport();
}

std::string ConstChecker::getVarKey(const clang::VarDecl* VD) const {
  if (!VD || !Context_) {
    return "";
  }

  clang::SourceManager& SM = Context_->getSourceManager();
  clang::SourceLocation loc = VD->getLocation();
  std::string file = SM.getFilename(loc).str();
  int line = SM.getExpansionLineNumber(loc);
  std::string name = VD->getName().str();

  return file + ":" + std::to_string(line) + ":" + name;
}

bool ConstChecker::VisitVarDecl(clang::VarDecl* VD) {
  if (!VD || !Context_) {
    return true;
  }

  if (isInSystemHeader(VD)) {
    return true;
  }

  if (clang::isa<clang::ParmVarDecl>(VD)) {
    return true;
  }

  clang::DeclContext* DC = VD->getDeclContext();
  if (clang::isa<clang::RecordDecl>(DC) || clang::isa<clang::CXXRecordDecl>(DC)) {
    return true;
  }

  std::string key = getVarKey(VD);
  if (key.empty()) {
    return true;
  }

  VarInfo info;
  info.name = VD->getName().str();
  info.type = VD->getType().getAsString();
  info.is_const = VD->getType().isConstQualified();
  info.is_constexpr = VD->isConstexpr();
  info.is_reference = VD->getType()->isReferenceType();
  info.is_pointer = VD->getType()->isPointerType();
  info.is_parameter = clang::isa<clang::ParmVarDecl>(VD);
  info.is_member = clang::isa<clang::FieldDecl>(VD);
  info.is_global = VD->hasGlobalStorage() && !VD->isLocalVarDecl() && !VD->isStaticLocal();

  // Check if variable has constant initializer
  info.has_const_init = VD->hasInit() && VD->getInit()->isConstantInitializer(*Context_, false);

  clang::SourceManager& SM = Context_->getSourceManager();
  clang::SourceLocation loc = VD->getLocation();
  info.file = SM.getFilename(loc).str();
  info.line = SM.getExpansionLineNumber(loc);
  info.column = SM.getExpansionColumnNumber(loc);

  variables_[key] = info;

  return true;
}

bool ConstChecker::VisitBinaryOperator(clang::BinaryOperator* BO) {
  if (!BO || !BO->isAssignmentOp()) {
    return true;
  }

  clang::Expr* lhs = BO->getLHS();
  if (!lhs) {
    return true;
  }

  lhs = lhs->IgnoreParenImpCasts();

  // Detect array modifications (arr[i] = x)
  if (auto* ASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(lhs)) {
    const clang::Expr* base = ASE->getBase()->IgnoreParenImpCasts();
    if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(base)) {
      if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
        std::string key = getVarKey(VD);
        if (!key.empty()) {
          modified_vars_.insert(key);
        }
      }
    }
  }
  
  // Regular assignment (var = x)
  if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(lhs)) {
    if (auto* varDecl = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
      std::string key = getVarKey(varDecl);
      if (!key.empty()) {
        modified_vars_.insert(key);
      }
    }
  }

  return true;
}

bool ConstChecker::VisitUnaryOperator(clang::UnaryOperator* UO) {
  if (!UO) {
    return true;
  }

  auto opcode = UO->getOpcode();
  if (opcode == clang::UO_AddrOf) {
    // Handle direct variable address: &var
    if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(UO->getSubExpr()->IgnoreParenImpCasts())) {
      if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
        std::string key = getVarKey(VD);
        if (!key.empty()) {
          modified_vars_.insert(key);
        }
      }
    }
    // Handle array element address: &arr[index] (marks the array as modified)
    else if (auto* ASE = llvm::dyn_cast<clang::ArraySubscriptExpr>(UO->getSubExpr()->IgnoreParenImpCasts())) {
      const clang::Expr* base = ASE->getBase()->IgnoreParenImpCasts();
      if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(base)) {
        if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
          std::string key = getVarKey(VD);
          if (!key.empty()) {
            modified_vars_.insert(key);
          }
        }
      }
    }
  } else if (opcode != clang::UO_PreInc && opcode != clang::UO_PreDec && opcode != clang::UO_PostInc &&
      opcode != clang::UO_PostDec) {
    return true;
  }

  clang::Expr* subExpr = UO->getSubExpr();
  if (!subExpr) {
    return true;
  }

  subExpr = subExpr->IgnoreParenImpCasts();

  if (auto* declRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
    if (auto* varDecl = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
      std::string key = getVarKey(varDecl);
      if (!key.empty()) {
        modified_vars_.insert(key);
      }
    }
  }

  return true;
}

bool ConstChecker::isInSystemHeader(clang::Decl* D) const {
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

bool ConstChecker::isBuiltinType(const std::string& type) const {
  static const std::vector<std::string> builtin_types = {
      "int",      "unsigned int",   "long",    "unsigned long", "long long",   "unsigned long long",
      "short",    "unsigned short", "char",    "unsigned char", "signed char", "float",
      "double",   "long double",    "bool",    "wchar_t",       "char16_t",    "char32_t",
      "size_t",   "int8_t",         "int16_t", "int32_t",       "int64_t",     "uint8_t",
      "uint16_t", "uint32_t",       "uint64_t"};

  std::string clean_type = type;
  size_t pos = clean_type.find("const ");
  while (pos != std::string::npos) {
    clean_type.erase(pos, 6);
    pos = clean_type.find("const ");
  }
  while (!clean_type.empty() && (clean_type.back() == ' ' || clean_type.back() == '&')) {
    clean_type.pop_back();
  }

  if (std::find(builtin_types.begin(), builtin_types.end(), clean_type) != builtin_types.end()) {
    return true;
  }

  size_t bracket_pos = clean_type.find('[');
  if (bracket_pos != std::string::npos) {
    std::string element_type = clean_type.substr(0, bracket_pos);
    while (!element_type.empty() && element_type.back() == ' ') {
      element_type.pop_back();
    }
    return std::find(builtin_types.begin(), builtin_types.end(), element_type) != builtin_types.end();
  }

  return false;
}

std::string ConstChecker::makeConstSuggestion(const VarInfo& info) const {
  return "const " + info.type + " " + info.name;
}

void ConstChecker::analyzeAndReport() {
  for (const auto& [key, info] : variables_) {
    if (info.is_constexpr) continue;
    
    if (modified_vars_.count(key) > 0) continue;
    if (info.is_parameter || info.is_member) continue;
    if (info.is_pointer) continue;
    
    bool can_be_constexpr = false;
    bool can_be_const = false;
    
    if (info.is_const) {
      if (info.has_const_init && isBuiltinType(info.type) && !info.is_reference) {
        can_be_constexpr = true;
      }
    } else {
      if (info.is_reference) {
        can_be_const = true;
      } else if (isBuiltinType(info.type) && info.has_const_init) {
        can_be_constexpr = true;
      }
    }
    
    if (can_be_constexpr || can_be_const) {
      LintIssue issue;
      issue.type = can_be_constexpr ? CheckType::CAN_BE_CONSTEXPR : CheckType::CAN_BE_CONST;
      issue.severity = Severity::HINT;
      issue.checker_name = "const";
      issue.name = info.name;
      issue.type_str = info.type;
      issue.file = info.file;
      issue.line = info.line;
      issue.column = info.column;
      issue.description = can_be_constexpr ? "Variable is never modified, consider making it constexpr" : "Variable is never modified, consider making it const";
      issue.suggestion = (can_be_constexpr ? "constexpr " : "const ") + info.type + " " + info.name;
      issue.fixable = true;
      Reporter_.add_issue(issue);
    }
  }
}

bool ConstChecker::apply_fixes(const std::string& filepath, const std::vector<LintIssue>& issues,
                               std::string& modified_content) {
  std::vector<std::string> lines;
  std::stringstream ss(modified_content);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  for (const auto& issue : issues) {
    if (issue.checker_name != "const" || issue.line <= 0 || issue.line > (int)lines.size()) {
      continue;
    }
    int idx = issue.line - 1;
    size_t pos = lines[idx].find(issue.name);
    if (pos != std::string::npos) {
      size_t type_pos = lines[idx].rfind(issue.type_str, pos);
      if (type_pos != std::string::npos) {
        lines[idx].insert(type_pos, "const ");
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

bool ConstChecker::VisitCallExpr(clang::CallExpr* CE) {
  if (!CE) return true;

  clang::FunctionDecl* FD = CE->getDirectCallee();
  if (!FD) return true;
  
  for (unsigned i = 0; i < CE->getNumArgs(); ++i) {
    clang::Expr* arg = CE->getArg(i)->IgnoreParenImpCasts();
    if (i < FD->getNumParams()) {
      clang::ParmVarDecl* paramDecl = FD->getParamDecl(i);
      if (!paramDecl) continue;
      
      clang::QualType paramType = paramDecl->getType();
      if (paramType->isPointerType() || paramType->isReferenceType()) {
        if (auto* UO = llvm::dyn_cast<clang::UnaryOperator>(arg)) {
          if (UO->getOpcode() == clang::UO_AddrOf) {
            if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(UO->getSubExpr())) {
              if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
                std::string key = getVarKey(VD);
                if (!key.empty()) {
                  modified_vars_.insert(key);
                }
              }
            }
          }
        } else if (auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(arg)) {
          if (auto* VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
            std::string key = getVarKey(VD);
            if (!key.empty()) {
              modified_vars_.insert(key);
            }
          }
        }
      }
    }
  }
  return true;
}

} // namespace lint
} // namespace codelint
