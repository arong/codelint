#include "lint/lint_visitor.h"

namespace codelint {
namespace lint {

LintVisitor::LintVisitor(clang::ASTContext *Context, IssueReporter &Reporter)
    : Context_(Context), Reporter_(Reporter) {}

bool LintVisitor::VisitVarDecl(clang::VarDecl *VD) {
    return true;
}

bool LintVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
    return true;
}

} // namespace lint
} // namespace codelint