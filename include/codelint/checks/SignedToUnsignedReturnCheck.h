#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>

namespace clang::tidy::codelint {

class SignedToUnsignedReturnCheck : public ClangTidyCheck {
public:
  SignedToUnsignedReturnCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus14 || LangOpts.CPlusPlus17 || LangOpts.CPlusPlus20;
  }

private:
  static bool isUnsignedType(const QualType& QT);
  static bool isSignedType(const QualType& QT);
  static std::string getFunctionName(const CallExpr* Call);

  void checkSignedToUnsignedConversion(const VarDecl* VarDeclPtr, const CallExpr* CallExprPtr,
                                       ASTContext* Ctx, const SourceManager& SrcMgr);
  void checkSignedToUnsignedAssignment(const Expr* LHSExprPtr, const CallExpr* CallExprPtr,
                                       ASTContext* Ctx, const SourceManager& SrcMgr);
};

} // namespace clang::tidy::codelint
