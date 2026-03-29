#pragma once

#include "lint/lint_checker.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "lint/issue_reporter.h"

namespace codelint {
namespace lint {

class InitChecker : public LintChecker, public clang::RecursiveASTVisitor<InitChecker> {
public:
    InitChecker() = default;
    ~InitChecker() = default;

    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "init"; }
    std::string description() const override { 
        return "Check variable initialization style"; 
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::INIT_UNINITIALIZED, 
                CheckType::INIT_EQUALS_SYNTAX, 
                CheckType::INIT_UNSIGNED_SUFFIX};
    }
    
    bool can_fix() const override { return true; }
    bool apply_fixes(const std::string& filepath,
                     const std::vector<LintIssue>& issues,
                     std::string& modified_content) override;

    bool VisitVarDecl(clang::VarDecl *VD);
    bool VisitFieldDecl(clang::FieldDecl *FD);
    void runOnAST(clang::ASTContext *Context);

private:
    clang::ASTContext *Context_ = nullptr;
    IssueReporter Reporter_;
    LintResult Result_;

    void checkUninitialized(clang::VarDecl *VD);
    void checkEqualsInit(clang::VarDecl *VD);
    void checkUnsignedSuffix(clang::VarDecl *VD);
    void checkFieldUninitialized(clang::FieldDecl *FD);
    void processRecordDecl(clang::RecordDecl *RD);
    bool shouldSkipAutoDeclaration(clang::VarDecl *VD);
    bool shouldSkipForLoopVariable(clang::VarDecl *VD);
    bool shouldSkipUnionMember(clang::VarDecl *VD);
    bool shouldSkipEnumClassWithoutZero(clang::VarDecl *VD);
    bool shouldSkipExternDeclaration(clang::VarDecl *VD);
    bool shouldSkipExceptionVariable(clang::VarDecl *VD);
    bool shouldSkipInitializerListConstructor(clang::VarDecl *VD);
    bool shouldSkipLambdaParameter(clang::VarDecl *VD);
    bool shouldSkipCatchVariableCopy(clang::VarDecl *VD);

    // Utility methods
    bool isInSystemHeader(clang::Decl *D) const;
    bool isUnsignedType(clang::QualType type) const;
    bool hasDigitValue(const std::string& value) const;
    bool hasUnsignedSuffix(const std::string& value) const;
    bool isMainFile(clang::Decl *D) const;
};

}  // namespace lint
}  // namespace codelint