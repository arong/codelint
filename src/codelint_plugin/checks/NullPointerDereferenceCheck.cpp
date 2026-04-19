#include "codelint/checks/NullPointerDereferenceCheck.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Analysis/AnalysisDeclContext.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>

namespace clang::tidy::codelint {

using namespace clang::ast_matchers;

bool NullPointerDereferenceCheck::isInSystemHeader(SourceLocation Loc, ASTContext* Ctx) {
  if (Ctx == nullptr) {
    return false;
  }
  const auto& SrcMgr = Ctx->getSourceManager();
  const SourceLocation ExpansionLoc{SrcMgr.getExpansionLoc(Loc)};
  return !SrcMgr.isInMainFile(ExpansionLoc);
}

bool NullPointerDereferenceCheck::isNullPointerLiteral(const Expr* E, ASTContext* Ctx) {
  if (E == nullptr) {
    return false;
  }

  E = E->IgnoreImplicit();

  if (isa<CXXNullPtrLiteralExpr>(E)) {
    return true;
  }

  if (const auto* IL = dyn_cast<IntegerLiteral>(E)) {
    if (IL->getValue().isZero()) {
      QualType Ty = IL->getType();
      if (Ty->isPointerType()) {
        return true;
      }
      if (Ty->isIntegerType() && !Ty->isBooleanType()) {
        if (Ctx != nullptr) {
          const auto& SrcMgr = Ctx->getSourceManager();
          StringRef Text = Lexer::getSourceText(CharSourceRange::getTokenRange(E->getSourceRange()),
                                                SrcMgr, Ctx->getLangOpts());
          if (Text.equals_insensitive("null") || Text.equals_insensitive("nullptr") ||
              Text == "0") {
            return true;
          }
        }
      }
    }
  }

  return false;
}

bool NullPointerDereferenceCheck::isPointerVariable(const VarDecl* VD) {
  if (VD == nullptr) {
    return false;
  }
  QualType Ty = VD->getType();
  return Ty->isPointerType() || Ty->isObjCObjectPointerType();
}

bool NullPointerDereferenceCheck::isPointerDereference(const Expr* E) {
  if (E == nullptr) {
    return false;
  }

  E = E->IgnoreImplicit();

  if (const auto* BO = dyn_cast<BinaryOperator>(E)) {
    if (BO->getOpcode() == BO_PtrMemD || BO->getOpcode() == BO_PtrMemI) {
      return true;
    }
  }

  if (const auto* UO = dyn_cast<UnaryOperator>(E)) {
    if (UO->getOpcode() == UO_Deref) {
      return true;
    }
  }

  if (isa<MemberExpr>(E)) {
    if (const auto* Base = cast<MemberExpr>(E)->getBase()) {
      Base = Base->IgnoreImplicit();
      if (Base->getType()->isPointerType()) {
        return true;
      }
    }
  }

  if (const auto* CE = dyn_cast<CXXOperatorCallExpr>(E)) {
    auto Op = CE->getOperator();
    if (Op == OO_Star || Op == OO_Arrow || Op == OO_ArrowStar) {
      return true;
    }
    if (Op == OO_Subscript) {
      if (CE->getNumArgs() > 0) {
        const Expr* Arg0 = CE->getArg(0);
        if (Arg0->getType()->isPointerType() || Arg0->getType()->isArrayType()) {
          return true;
        }
      }
    }
  }

  if (isa<ArraySubscriptExpr>(E)) {
    return true;
  }

  return false;
}

const VarDecl* NullPointerDereferenceCheck::getReferencedVar(const Expr* E) {
  if (E == nullptr) {
    return nullptr;
  }

  E = E->IgnoreImplicit();

  if (const auto* DRE = dyn_cast<DeclRefExpr>(E)) {
    if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      if (isPointerVariable(VD)) {
        return VD;
      }
    }
  }

  if (const auto* ME = dyn_cast<MemberExpr>(E)) {
    if (const auto* FD = dyn_cast<FieldDecl>(ME->getMemberDecl())) {
      if (FD->getType()->isPointerType()) {
        return nullptr;
      }
    }
    const Expr* Base = ME->getBase();
    if (const auto* BaseDRE = dyn_cast<DeclRefExpr>(Base->IgnoreImplicit())) {
      if (const auto* VD = dyn_cast<VarDecl>(BaseDRE->getDecl())) {
        return VD;
      }
    }
  }

  return nullptr;
}

NullState NullPointerDereferenceCheck::getExprNullState(const Expr* E, ASTContext* Ctx) {
  if (E == nullptr) {
    return NullState::Unknown;
  }

  E = E->IgnoreImplicit();

  if (isNullPointerLiteral(E, Ctx)) {
    return NullState::Null;
  }

  if (isa<CXXNewExpr>(E)) {
    return NullState::NonNull;
  }

  if (const auto* CE = dyn_cast<CallExpr>(E)) {
    const FunctionDecl* FD = CE->getDirectCallee();
    if (FD != nullptr) {
      if (FD->hasAttr<ReturnsNonNullAttr>()) {
        return NullState::NonNull;
      }
      StringRef Name = FD->getName();
      if (Name == "malloc" || Name == "calloc" || Name == "realloc") {
        return NullState::MayBeNull;
      }
      if (Name.starts_with("alloc") || Name.starts_with("create")) {
        return NullState::MayBeNull;
      }
      if (Name == "get" || Name == "find" || Name == "lookup") {
        return NullState::MayBeNull;
      }
    }
    return NullState::MayBeNull;
  }

  if (const auto* DRE = dyn_cast<DeclRefExpr>(E)) {
    if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      if (VD->getType()->isPointerType()) {
        return NullState::Unknown;
      }
    }
  }

  return NullState::Unknown;
}

NullState NullPointerDereferenceCheck::mergeStates(NullState A, NullState B) {
  if (A == B) {
    return A;
  }
  if (A == NullState::Unknown || B == NullState::Unknown) {
    return NullState::Unknown;
  }
  return NullState::MayBeNull;
}

bool NullPointerDereferenceCheck::isPointerCheck(const Stmt* S, const VarDecl*& CheckedVar,
                                                 bool& IsNonNullCheck) {
  CheckedVar = nullptr;
  IsNonNullCheck = false;

  if (S == nullptr) {
    return false;
  }

  const Expr* Condition = nullptr;
  if (const auto* IS = dyn_cast<IfStmt>(S)) {
    Condition = IS->getCond();
  } else if (const auto* WS = dyn_cast<WhileStmt>(S)) {
    Condition = WS->getCond();
  } else if (const auto* FS = dyn_cast<ForStmt>(S)) {
    Condition = FS->getCond();
  } else if (const auto* CondOp = dyn_cast<ConditionalOperator>(S)) {
    Condition = CondOp->getCond();
  } else if (const auto* BO = dyn_cast<BinaryOperator>(S)) {
    if (BO->isLogicalOp()) {
      Condition = BO;
    }
  }

  if (Condition == nullptr) {
    return false;
  }

  Condition = Condition->IgnoreImplicit();

  if (const auto* DRE = dyn_cast<DeclRefExpr>(Condition)) {
    if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      if (isPointerVariable(VD)) {
        CheckedVar = VD;
        IsNonNullCheck = true;
        return true;
      }
    }
  }

  if (const auto* UO = dyn_cast<UnaryOperator>(Condition)) {
    if (UO->getOpcode() == UO_LNot) {
      const Expr* SubExpr = UO->getSubExpr()->IgnoreImplicit();
      if (const auto* DRE = dyn_cast<DeclRefExpr>(SubExpr)) {
        if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
          if (isPointerVariable(VD)) {
            CheckedVar = VD;
            IsNonNullCheck = false;
            return true;
          }
        }
      }
    }
  }

  if (const auto* BO = dyn_cast<BinaryOperator>(Condition)) {
    if (BO->isEqualityOp() || BO->isRelationalOp()) {
      const Expr* LHS = BO->getLHS()->IgnoreImplicit();
      const Expr* RHS = BO->getRHS()->IgnoreImplicit();

      const VarDecl* Var = nullptr;
      bool VarIsNullCompare = false;
      bool VarIsLeft = false;

      if (const auto* DRE = dyn_cast<DeclRefExpr>(LHS)) {
        if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
          if (isPointerVariable(VD)) {
            Var = VD;
            VarIsLeft = true;
            VarIsNullCompare = isNullPointerLiteral(RHS, nullptr);
          }
        }
      } else if (const auto* DRE = dyn_cast<DeclRefExpr>(RHS)) {
        if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
          if (isPointerVariable(VD)) {
            Var = VD;
            VarIsLeft = false;
            VarIsNullCompare = isNullPointerLiteral(LHS, nullptr);
          }
        }
      }

      if (Var != nullptr) {
        CheckedVar = Var;
        bool IsEq = BO->getOpcode() == BO_EQ;
        if (VarIsNullCompare) {
          IsNonNullCheck = IsEq ? false : true;
        }
        return true;
      }
    }
  }

  return false;
}

void NullPointerDereferenceCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(functionDecl(hasBody(compoundStmt().bind("body"))).bind("function"), this);
}

void NullPointerDereferenceCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  if (const auto* Func = Result.Nodes.getNodeAs<FunctionDecl>("function")) {
    if (Func == nullptr || Func->getBody() == nullptr) {
      return;
    }
    if (isInSystemHeader(Func->getLocation(), Result.Context)) {
      return;
    }
    analyzeFunction(Func, Result.Context);
  }
}

void NullPointerDereferenceCheck::analyzeFunction(const FunctionDecl* Func, ASTContext* Ctx) {
  if (Func == nullptr || Ctx == nullptr || Func->getBody() == nullptr) {
    return;
  }

  std::unique_ptr<CFG> Cfg = CFG::buildCFG(Func, Func->getBody(), Ctx, CFG::BuildOptions());

  if (Cfg == nullptr || Cfg->size() == 0) {
    return;
  }

  runDataFlowAnalysis(Cfg.get(), Ctx);
}

NullPointerDereferenceCheck::BlockState NullPointerDereferenceCheck::computeBlockEntryState(
    const CFGBlock* Block, const llvm::DenseMap<const CFGBlock*, BlockState>& BlockStates,
    ASTContext* Ctx) {

  if (Block == nullptr) {
    BlockState EmptyState;
    EmptyState.IsValid = true;
    return EmptyState;
  }

  BlockState Result;
  Result.IsValid = true;

  llvm::SmallVector<const CFGBlock*, 4> Preds;
  for (const CFGBlock* Pred : Block->preds()) {
    if (Pred != nullptr && BlockStates.contains(Pred)) {
      Preds.push_back(Pred);
    }
  }

  if (Preds.empty()) {
    return Result;
  }

  const BlockState& FirstPredState = BlockStates.find(Preds[0])->second;
  Result.VarStates = FirstPredState.VarStates;

  for (size_t I = 1; I < Preds.size(); ++I) {
    const BlockState& PredState = BlockStates.find(Preds[I])->second;
    for (auto& [Var, State] : Result.VarStates) {
      if (PredState.VarStates.contains(Var)) {
        State = mergeStates(State, PredState.VarStates.find(Var)->second);
      } else {
        State = NullState::Unknown;
      }
    }
    for (const auto& [Var, State] : PredState.VarStates) {
      if (!Result.VarStates.contains(Var)) {
        Result.VarStates[Var] = State;
      }
    }
  }

  bool IsNonNullBranch = false;
  bool IsNullBranch = false;
  const VarDecl* CheckedVar = nullptr;

  for (const CFGBlock* Pred : Block->preds()) {
    if (Pred == nullptr) {
      continue;
    }
    const Stmt* TermStmt = Pred->getTerminatorStmt();
    if (TermStmt != nullptr) {
      bool IsNonNullCheck = false;
      if (isPointerCheck(TermStmt, CheckedVar, IsNonNullCheck)) {
        unsigned SuccIndex = 0;
        for (const CFGBlock* Succ : Pred->succs()) {
          if (Succ == Block) {
            break;
          }
          SuccIndex++;
        }
        if (SuccIndex == 0) {
          IsNonNullBranch = IsNonNullCheck;
          IsNullBranch = !IsNonNullCheck;
        } else if (SuccIndex == 1) {
          IsNonNullBranch = !IsNonNullCheck;
          IsNullBranch = IsNonNullCheck;
        }
        break;
      }
    }
  }

  if (CheckedVar != nullptr) {
    if (IsNonNullBranch) {
      Result.VarStates[CheckedVar] = NullState::NonNull;
    } else if (IsNullBranch) {
      Result.VarStates[CheckedVar] = NullState::Null;
    }
  }

  return Result;
}

NullPointerDereferenceCheck::BlockState
NullPointerDereferenceCheck::transferBlock(const CFGBlock* Block, BlockState EntryState,
                                           ASTContext* Ctx) {
  if (Block == nullptr || Ctx == nullptr) {
    return EntryState;
  }

  BlockState CurrentState = EntryState;

  for (const CFGElement& Elem : *Block) {
    if (Elem.getKind() != CFGElement::Statement) {
      continue;
    }

    const Stmt* S = Elem.castAs<CFGStmt>().getStmt();
    if (S == nullptr) {
      continue;
    }

    if (const auto* DS = dyn_cast<DeclStmt>(S)) {
      for (const Decl* D : DS->getDeclGroup()) {
        if (const auto* VD = dyn_cast<VarDecl>(D)) {
          if (isPointerVariable(VD) && VD->hasInit()) {
            const Expr* Init = VD->getInit();
            NullState State = getExprNullState(Init, Ctx);
            CurrentState.VarStates[VD] = State;
          } else if (isPointerVariable(VD) && !VD->hasInit()) {
            CurrentState.VarStates[VD] = NullState::MayBeNull;
          }
        }
      }
    }

    if (const auto* BO = dyn_cast<BinaryOperator>(S)) {
      if (BO->isAssignmentOp()) {
        const Expr* LHS = BO->getLHS()->IgnoreImplicit();
        const Expr* RHS = BO->getRHS()->IgnoreImplicit();

        const VarDecl* AssignedVar = nullptr;
        if (const auto* DRE = dyn_cast<DeclRefExpr>(LHS)) {
          if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
            if (isPointerVariable(VD)) {
              AssignedVar = VD;
            }
          }
        }

        if (AssignedVar != nullptr) {
          NullState State = getExprNullState(RHS, Ctx);
          CurrentState.VarStates[AssignedVar] = State;
        }
      }
    }

    if (const auto* CE = dyn_cast<CallExpr>(S)) {
      if (CE->getDirectCallee() != nullptr) {
        StringRef Name = CE->getDirectCallee()->getName();
        if (Name == "assert" || Name == "ASSERT") {
          if (CE->getNumArgs() > 0) {
            const Expr* Arg = CE->getArg(0)->IgnoreImplicit();
            const VarDecl* AssertedVar = nullptr;
            bool IsNonNullAssert = false;
            if (const auto* DRE = dyn_cast<DeclRefExpr>(Arg)) {
              if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                if (isPointerVariable(VD)) {
                  AssertedVar = VD;
                  IsNonNullAssert = true;
                }
              }
            }
            if (const auto* UO = dyn_cast<UnaryOperator>(Arg)) {
              if (UO->getOpcode() == UO_LNot) {
                const Expr* SubExpr = UO->getSubExpr()->IgnoreImplicit();
                if (const auto* DRE = dyn_cast<DeclRefExpr>(SubExpr)) {
                  if (const auto* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                    if (isNullPointerLiteral(SubExpr, Ctx) || VD->getType()->isPointerType()) {
                      AssertedVar = VD;
                      IsNonNullAssert = true;
                    }
                  }
                }
              }
            }
            if (AssertedVar != nullptr && IsNonNullAssert) {
              CurrentState.VarStates[AssertedVar] = NullState::NonNull;
            }
          }
        }
      }
    }
  }

  return CurrentState;
}

void NullPointerDereferenceCheck::checkBlockForDereferences(const CFGBlock* Block,
                                                            const BlockState& State,
                                                            ASTContext* Ctx) {
  if (Block == nullptr || Ctx == nullptr) {
    return;
  }

  for (const CFGElement& Elem : *Block) {
    if (Elem.getKind() != CFGElement::Statement) {
      continue;
    }

    const Stmt* S = Elem.castAs<CFGStmt>().getStmt();
    if (S == nullptr) {
      continue;
    }

    llvm::SmallVector<const Expr*, 8> Derefs;

    if (const auto* UO = dyn_cast<UnaryOperator>(S)) {
      if (UO->getOpcode() == UO_Deref) {
        Derefs.push_back(UO->getSubExpr());
      }
    }

    if (const auto* BO = dyn_cast<BinaryOperator>(S)) {
      if (BO->getOpcode() == BO_PtrMemD || BO->getOpcode() == BO_PtrMemI) {
        Derefs.push_back(BO->getLHS());
      }
    }

    if (const auto* CE = dyn_cast<CXXOperatorCallExpr>(S)) {
      auto Op = CE->getOperator();
      if (Op == OO_Star || Op == OO_Arrow || Op == OO_ArrowStar) {
        if (CE->getNumArgs() > 0) {
          Derefs.push_back(CE->getArg(0));
        }
      }
      if (Op == OO_Subscript) {
        if (CE->getNumArgs() > 0) {
          const Expr* Arg0 = CE->getArg(0);
          if (Arg0->getType()->isPointerType()) {
            Derefs.push_back(Arg0);
          }
        }
      }
    }

    if (const auto* ME = dyn_cast<MemberExpr>(S)) {
      const Expr* Base = ME->getBase();
      if (Base->getType()->isPointerType()) {
        Derefs.push_back(Base);
      }
    }

    if (const auto* ASE = dyn_cast<ArraySubscriptExpr>(S)) {
      const Expr* Base = ASE->getBase();
      if (Base->getType()->isPointerType()) {
        Derefs.push_back(Base);
      }
    }

    if (const auto* BO = dyn_cast<BinaryOperator>(S)) {
      if (BO->isAssignmentOp() && isPointerDereference(BO->getLHS())) {
        const Expr* PointerExpr = nullptr;
        if (const auto* UO = dyn_cast<UnaryOperator>(BO->getLHS())) {
          PointerExpr = UO->getSubExpr();
        } else if (const auto* ASE = dyn_cast<ArraySubscriptExpr>(BO->getLHS())) {
          PointerExpr = ASE->getBase();
        }
        if (PointerExpr != nullptr) {
          Derefs.push_back(PointerExpr);
        }
      }
    }

    for (const Expr* PointerExpr : Derefs) {
      PointerExpr = PointerExpr->IgnoreImplicit();

      const VarDecl* PointerVar = getReferencedVar(PointerExpr);
      if (PointerVar == nullptr) {
        continue;
      }

      NullState PointerState = NullState::Unknown;
      if (State.VarStates.contains(PointerVar)) {
        PointerState = State.VarStates.find(PointerVar)->second;
      }

      if (PointerState == NullState::Null || PointerState == NullState::MayBeNull) {
        reportDereference(PointerExpr->getExprLoc(), PointerExpr, PointerState);
      }
    }
  }
}

void NullPointerDereferenceCheck::reportDereference(SourceLocation Loc, const Expr* PointerExpr,
                                                    NullState State) {
  if (State == NullState::Null) {
    diag(Loc, "dereference of null pointer", DiagnosticIDs::Error);
  } else if (State == NullState::MayBeNull) {
    diag(Loc, "potential dereference of null pointer");
  }
}

void NullPointerDereferenceCheck::runDataFlowAnalysis(const CFG* Cfg, ASTContext* Ctx) {
  if (Cfg == nullptr || Ctx == nullptr) {
    return;
  }

  llvm::DenseMap<const CFGBlock*, BlockState> BlockStates;
  llvm::SmallVector<const CFGBlock*, 16> WorkList;

  for (const CFGBlock* Block : *Cfg) {
    if (Block == nullptr) {
      continue;
    }
    WorkList.push_back(Block);
  }

  bool Changed = true;
  unsigned Iterations = 0;
  const unsigned MaxIterations = 100;

  while (Changed && Iterations < MaxIterations) {
    Changed = false;
    Iterations++;

    for (const CFGBlock* Block : WorkList) {
      if (Block == nullptr) {
        continue;
      }

      BlockState EntryState = computeBlockEntryState(Block, BlockStates, Ctx);
      BlockState ExitState = transferBlock(Block, EntryState, Ctx);

      if (!BlockStates.contains(Block)) {
        BlockStates[Block] = ExitState;
        Changed = true;
      } else {
        const BlockState& OldState = BlockStates.find(Block)->second;
        bool StatesChanged = false;
        for (const auto& [Var, State] : ExitState.VarStates) {
          if (!OldState.VarStates.contains(Var)) {
            StatesChanged = true;
            break;
          }
          if (OldState.VarStates.find(Var)->second != State) {
            NullState Merged = mergeStates(OldState.VarStates.find(Var)->second, State);
            if (Merged != OldState.VarStates.find(Var)->second) {
              StatesChanged = true;
              break;
            }
          }
        }
        for (const auto& [Var, State] : OldState.VarStates) {
          if (!ExitState.VarStates.contains(Var)) {
            StatesChanged = true;
            break;
          }
        }
        if (StatesChanged) {
          BlockStates[Block] = ExitState;
          Changed = true;
        }
      }
    }
  }

  for (const CFGBlock* Block : *Cfg) {
    if (Block == nullptr) {
      continue;
    }
    if (!BlockStates.contains(Block)) {
      continue;
    }
    BlockState EntryState = computeBlockEntryState(Block, BlockStates, Ctx);
    checkBlockForDereferences(Block, EntryState, Ctx);
  }
}

} // namespace clang::tidy::codelint
