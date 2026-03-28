#ifndef CNDY_LINT_VISITOR_H
#define CNDY_LINT_VISITOR_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

namespace codelint {
namespace lint {

class IssueReporter;

// LintVisitor traverses the AST using RecursiveASTVisitor pattern
class LintVisitor : public clang::RecursiveASTVisitor<LintVisitor> {
public:
    explicit LintVisitor(clang::ASTContext *Context, IssueReporter &Reporter);
    ~LintVisitor() = default;

    // Visit methods for AST nodes
    bool VisitVarDecl(clang::VarDecl *VD);
    bool VisitFunctionDecl(clang::FunctionDecl *FD);

private:
    clang::ASTContext *Context_;
    IssueReporter &Reporter_;
};

} // namespace lint
} // namespace cndy

#endif // CNDY_LINT_VISITOR_H