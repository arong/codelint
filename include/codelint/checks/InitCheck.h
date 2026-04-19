#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace clang::tidy::codelint {

class InitCheck : public ClangTidyCheck {
public:
  InitCheck(StringRef Name, ClangTidyContext* Context) : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  static bool isInSystemHeader(SourceLocation Loc, ASTContext* Ctx);

  void checkUninitialized(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkUninitializedField(const FieldDecl* FieldDeclPtr, ASTContext* Ctx);
  void checkEqualsInit(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkUnsignedSuffix(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkEqualsBraceInit(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  void checkUninitializedMemberVariablesInConstructors(const CXXConstructorDecl* Ctor,
                                                       ASTContext* Ctx);

  static bool shouldSkipAuto(const VarDecl* VarDeclPtr);
  static bool isAutoType(const VarDecl* VarDeclPtr);
  static bool shouldSkipUnion(const VarDecl* VarDeclPtr);
  static bool shouldSkipExtern(const VarDecl* VarDeclPtr);
  static bool shouldSkipEnumClass(const VarDecl* VarDeclPtr);
  static bool isEnumZeroValidType(const Type* TypePtr);
  static bool hasExplicitInitializer(const VarDecl* VarDeclPtr);
  static bool hasExplicitInitializer(const FieldDecl* FieldDeclPtr);
  static bool isInsideMacro(const VarDecl* VarDeclPtr, ASTContext* Ctx);
  [[nodiscard]] static bool hasNonTrivialDefaultConstructor(QualType QualTypeRef);
};

} // namespace clang::tidy::codelint
