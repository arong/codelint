#pragma once

#include <clang-tidy/ClangTidyCheck.h>
#include <clang/Analysis/CFG.h>
#include <llvm/ADT/DenseMap.h>

namespace clang::tidy::codelint {

enum class NullState {
  Unknown,
  Null,
  NonNull,
  MayBeNull,
};

class NullPointerDereferenceCheck : public ClangTidyCheck {
public:
  NullPointerDereferenceCheck(StringRef Name, ClangTidyContext* Context)
      : ClangTidyCheck(Name, Context) {
  }

  void registerMatchers(ast_matchers::MatchFinder* Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult& Result) override;

  [[nodiscard]] bool isLanguageVersionSupported(const LangOptions& LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void analyzeFunction(const FunctionDecl* Func, ASTContext* Ctx);

  static bool isInSystemHeader(SourceLocation Loc, ASTContext* Ctx);

  static NullState getExprNullState(const Expr* E, ASTContext* Ctx);

  static bool isPointerDereference(const Expr* E);
  static bool isNullPointerLiteral(const Expr* E, ASTContext* Ctx);

  void reportDereference(SourceLocation Loc, const Expr* PointerExpr, NullState State);

  using NullStateMap = llvm::DenseMap<const VarDecl*, NullState>;

  struct BlockState {
    NullStateMap VarStates;
    bool IsValid = true;
  };

  void runDataFlowAnalysis(const CFG* Cfg, ASTContext* Ctx);

  BlockState computeBlockEntryState(const CFGBlock* Block,
                                    const llvm::DenseMap<const CFGBlock*, BlockState>& BlockStates,
                                    ASTContext* Ctx);

  BlockState transferBlock(const CFGBlock* Block, BlockState EntryState, ASTContext* Ctx);

  void checkBlockForDereferences(const CFGBlock* Block, const BlockState& State, ASTContext* Ctx);

  void checkExprForDereferences(const Expr* E, const BlockState& State, ASTContext* Ctx);

  static NullState mergeStates(NullState A, NullState B);

  static const VarDecl* getReferencedVar(const Expr* E);

  static bool isPointerVariable(const VarDecl* VD);

  static bool isPointerCheck(const Stmt* S, const VarDecl*& CheckedVar, bool& IsNonNullCheck);
};

} // namespace clang::tidy::codelint
