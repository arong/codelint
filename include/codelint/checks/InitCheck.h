#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy {
namespace codelint {

class InitCheck : public ClangTidyCheck {
public:
  InitCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void checkUninitialized(const VarDecl* VD, ASTContext* Ctx);
  void checkUninitializedField(const FieldDecl* FD, ASTContext* Ctx);
  void checkEqualsInit(const VarDecl* VD, ASTContext* Ctx);
  void checkUnsignedSuffix(const VarDecl* VD, ASTContext* Ctx);
  void checkEqualsBraceInit(const VarDecl* VD, ASTContext* Ctx);
  void checkUninitializedMemberVariablesInConstructors(const CXXConstructorDecl* Ctor,
                                                       ASTContext* Ctx);

  bool shouldSkipAuto(const VarDecl* VD);
  bool shouldSkipUnion(const VarDecl* VD);
  bool shouldSkipExtern(const VarDecl* VD);
  bool shouldSkipEnumClass(const VarDecl* VD);
  bool isBraceInit(const VarDecl* VD);
  bool hasExplicitInitializer(const VarDecl* VD);
  bool hasExplicitInitializer(const FieldDecl* FD);
  bool isInsideMacro(const VarDecl* VD, ASTContext* Ctx);
};

} // namespace codelint
} // namespace clang::tidy
