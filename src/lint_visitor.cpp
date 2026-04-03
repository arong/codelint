#include "lint/lint_visitor.h"
#include "lint/issue_reporter.h"
#include "lint/lint_types.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/Analyses/Dominators.h"
#include "clang/Analysis/CFG.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/StringRef.h"

namespace codelint {
namespace lint {

LintVisitor::LintVisitor(clang::ASTContext* Context, IssueReporter& Reporter)
    : Context_(Context), Reporter_(Reporter) {
}

bool LintVisitor::VisitVarDecl(clang::VarDecl* VD) {
  if (!VD) {
    return true;
  }

  if (shouldSkipAutoDeclaration(VD) || shouldSkipForLoopVariable(VD) || shouldSkipUnionMember(VD) ||
      shouldSkipEnumClassWithoutZero(VD) || shouldSkipExternDeclaration(VD) ||
      shouldSkipExceptionVariable(VD)) {
    return true;
  }

  checkVariableInitialization(VD);

  return true;
}

bool LintVisitor::VisitFunctionDecl(clang::FunctionDecl* FD) {
  return true;
}

void LintVisitor::checkVariableInitialization(clang::VarDecl* VD) {
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

  clang::Expr* init = VD->getInit();

  if (!init) {
    std::string suggestion_prefix;
    if (canBeConstexpr(VD)) {
      suggestion_prefix = "constexpr ";
    } else if (canBeConst(VD)) {
      suggestion_prefix = "const ";
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
    issue.suggestion = suggestion_prefix + type_str + " " + name + "{};";
    issue.fixable = true;

    Reporter_.add_issue(issue);
    return;
  }

  bool is_brace_init = clang::isa<clang::InitListExpr>(init);

  if (is_brace_init) {
    if (canBeConstexpr(VD)) {
      LintIssue issue;
      issue.type = CheckType::CAN_BE_CONSTEXPR;
      issue.severity = Severity::HINT;
      issue.checker_name = "init";
      issue.name = name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.column = column;
      issue.description = "Variable can be constexpr";
      issue.suggestion = "constexpr " + type_str + " " + name + "{...};";
      issue.fixable = true;

      Reporter_.add_issue(issue);
    } else if (canBeConst(VD)) {
      LintIssue issue;
      issue.type = CheckType::CAN_BE_CONST;
      issue.severity = Severity::HINT;
      issue.checker_name = "init";
      issue.name = name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.column = column;
      issue.description = "Variable can be const";
      issue.suggestion = "const " + type_str + " " + name + "{...};";
      issue.fixable = true;

      Reporter_.add_issue(issue);
    }
    return;
  }

  if (VD->getInitStyle() == clang::VarDecl::CInit) {
    if (canBeConstexpr(VD)) {
      LintIssue issue;
      issue.type = CheckType::CAN_BE_CONSTEXPR;
      issue.severity = Severity::WARNING;
      issue.checker_name = "init";
      issue.name = name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.column = column;
      issue.description = "Variable should use constexpr with '{}' syntax";
      issue.suggestion = "constexpr " + type_str + " " + name + "{...};";
      issue.fixable = true;

      Reporter_.add_issue(issue);
    } else if (canBeConst(VD)) {
      LintIssue issue;
      issue.type = CheckType::CAN_BE_CONST;
      issue.severity = Severity::WARNING;
      issue.checker_name = "init";
      issue.name = name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.column = column;
      issue.description = "Variable should use const with '{}' syntax";
      issue.suggestion = "const " + type_str + " " + name + "{...};";
      issue.fixable = true;

      Reporter_.add_issue(issue);
    } else {
      LintIssue issue;
      issue.type = CheckType::INIT_EQUALS_SYNTAX;
      issue.severity = Severity::WARNING;
      issue.checker_name = "init";
      issue.name = name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.column = column;
      issue.description = "Variable should use '{}' syntax for initialization";
      issue.suggestion = type_str + " " + name + "{...};";
      issue.fixable = true;

      Reporter_.add_issue(issue);
    }
  }

  if (isUnsignedType(type)) {
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
    issue.suggestion = "";
    issue.fixable = false;

    Reporter_.add_issue(issue);
  }
}

bool LintVisitor::isUnsignedType(clang::QualType type) const {
  std::string type_str = type.getAsString();
  return type_str.find("unsigned") != std::string::npos;
}

bool LintVisitor::shouldSkipAutoDeclaration(clang::VarDecl* VD) {
  clang::QualType type = VD->getType();
  if (type->isUndeducedAutoType()) {
    return true;
  }

  clang::SourceManager& SM = Context_->getSourceManager();
  clang::SourceLocation loc = VD->getBeginLoc();
  bool invalid = false;
  llvm::StringRef buffer = SM.getBufferData(SM.getFileID(loc), &invalid);
  if (invalid) {
    return false;
  }

  unsigned offset = SM.getFileOffset(loc);
  if (offset < 5) {
    return false;
  }

  llvm::StringRef prefix = buffer.substr(offset - 5, 5);
  if (prefix.contains("auto")) {
    return true;
  }

  return false;
}

bool LintVisitor::shouldSkipForLoopVariable(clang::VarDecl* VD) {
  clang::DynTypedNodeList parents = Context_->getParents(*VD);
  for (const auto& parent : parents) {
    if (parent.get<clang::ForStmt>() || parent.get<clang::CXXForRangeStmt>()) {
      return true;
    }
  }
  return false;
}

bool LintVisitor::shouldSkipUnionMember(clang::VarDecl* VD) {
  clang::DeclContext* DC = VD->getDeclContext();
  if (clang::RecordDecl* RD = clang::dyn_cast<clang::RecordDecl>(DC)) {
    return RD->isUnion();
  }
  return false;
}

bool LintVisitor::shouldSkipEnumClassWithoutZero(clang::VarDecl* VD) {
  clang::QualType type = VD->getType();
  const clang::EnumType* ET = type->getAs<clang::EnumType>();
  if (!ET) {
    return false;
  }

  clang::EnumDecl* ED = ET->getDecl();
  if (!ED || !ED->isScoped()) {
    return false;
  }

  bool hasZeroValue = false;
  for (clang::EnumConstantDecl* ECD : ED->enumerators()) {
    if (ECD->getInitVal() == 0) {
      hasZeroValue = true;
      break;
    }
  }

  return !hasZeroValue;
}

bool LintVisitor::shouldSkipExternDeclaration(clang::VarDecl* VD) {
  return VD->getStorageClass() == clang::SC_Extern;
}

bool LintVisitor::shouldSkipExceptionVariable(clang::VarDecl* VD) {
  return VD->isExceptionVariable();
}

bool LintVisitor::canBeConst(clang::VarDecl* VD) {
  if (VD->getType().isConstQualified()) {
    return false;
  }

  if (!VD->isLocalVarDecl()) {
    return false;
  }

  if (clang::isa<clang::ParmVarDecl>(VD)) {
    return false;
  }

  clang::FunctionDecl* func = nullptr;
  clang::DeclContext* DC = VD->getDeclContext();
  while (DC) {
    if (auto* FD = clang::dyn_cast<clang::FunctionDecl>(DC)) {
      func = FD;
      break;
    }
    DC = DC->getParent();
  }

  if (!func || !func->hasBody()) {
    return false;
  }

  clang::CFG::BuildOptions options;
  std::unique_ptr<clang::CFG> cfg = clang::CFG::buildCFG(func, func->getBody(), Context_, options);

  if (!cfg) {
    return false;
  }

  for (const clang::CFGBlock* block : *cfg) {
    for (const clang::CFGElement& elem : *block) {
      if (elem.getKind() == clang::CFGElement::Statement) {
        if (auto stmt = elem.getAs<clang::CFGStmt>()) {
          if (isAssignmentToVar(stmt->getStmt(), VD)) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

bool LintVisitor::canBeConstexpr(clang::VarDecl* VD) {
  if (!canBeConst(VD)) {
    return false;
  }

  const clang::Expr* init = VD->getInit();
  if (!init) {
    return false;
  }

  clang::Expr::EvalResult result;
  if (init->EvaluateAsRValue(result, *Context_)) {
    clang::QualType type = VD->getType();
    if (type->isPointerType() || type->isReferenceType()) {
      return false;
    }
    return true;
  }

  return false;
}

bool LintVisitor::isAssignmentToVar(const clang::Stmt* S, clang::VarDecl* VD) {
  if (auto* BO = clang::dyn_cast<clang::BinaryOperator>(S)) {
    if (BO->isAssignmentOp()) {
      const clang::Expr* lhs = BO->getLHS();
      if (auto* DRE = clang::dyn_cast<clang::DeclRefExpr>(lhs)) {
        if (DRE->getDecl() == VD) {
          return true;
        }
      }
    }
  }

  if (auto* UO = clang::dyn_cast<clang::UnaryOperator>(S)) {
    if (UO->isIncrementDecrementOp()) {
      const clang::Expr* sub = UO->getSubExpr();
      if (auto* DRE = clang::dyn_cast<clang::DeclRefExpr>(sub)) {
        if (DRE->getDecl() == VD) {
          return true;
        }
      }
    }
  }

  return false;
}

} // namespace lint
} // namespace codelint
