#pragma once

#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "lint/issue_reporter.h"

namespace codelint {
namespace lint {

class GlobalChecker : public LintChecker, public clang::RecursiveASTVisitor<GlobalChecker> {
public:
    GlobalChecker() = default;
    ~GlobalChecker() = default;

    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "global"; }
    std::string description() const override { 
        return "Detect global variables"; 
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::GLOBAL_VARIABLE};
    }
    
    bool can_fix() const override { return false; }

    bool VisitVarDecl(clang::VarDecl *VD);

private:
    clang::ASTContext *Context_ = nullptr;
    IssueReporter Reporter_;
    LintResult Result_;

    bool isGlobalVariable(clang::VarDecl *VD) const;
    bool isInSystemHeader(clang::VarDecl *VD) const;
    bool isExternDeclaration(clang::VarDecl *VD) const;
    void reportGlobalVariable(clang::VarDecl *VD);
    void runOnAST(clang::ASTContext *Context);
};

}  // namespace lint
}  // namespace codelint