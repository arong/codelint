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

    // Helper methods for variable initialization checking
    void checkVariableInitialization(clang::VarDecl *VD);
    bool isUnsignedType(clang::QualType type) const;

    // Skip rule helper methods
    bool shouldSkipAutoDeclaration(clang::VarDecl *VD);
    bool shouldSkipForLoopVariable(clang::VarDecl *VD);
    bool shouldSkipUnionMember(clang::VarDecl *VD);
    bool shouldSkipEnumClassWithoutZero(clang::VarDecl *VD);
    bool shouldSkipExternDeclaration(clang::VarDecl *VD);
    bool shouldSkipExceptionVariable(clang::VarDecl *VD);

    // Const/constexpr suggestion helper methods
    bool canBeConst(clang::VarDecl *VD);
    bool canBeConstexpr(clang::VarDecl *VD);
    bool isAssignmentToVar(const clang::Stmt *S, clang::VarDecl *VD);
};

} // namespace lint
} // namespace cndy

#endif // CNDY_LINT_VISITOR_H