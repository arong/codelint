#include "codelint/checks/LogTagMismatchCheck.h"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>
#include <regex>

namespace clang::tidy::codelint {

using namespace ast_matchers;

LogTagMismatchCheck::LogTagMismatchCheck(StringRef Name, ClangTidyContext* Context)
    : ClangTidyCheck(Name, Context), LogMacroNames(Options.get("LogMacroNames", "*LOG*,*log*")),
      AllowQualifiedName(Options.get("AllowQualifiedName", true)) {
}

void LogTagMismatchCheck::storeOptions(ClangTidyOptions::OptionMap& Opts) {
  Options.store(Opts, "LogMacroNames", LogMacroNames);
  Options.store(Opts, "AllowQualifiedName", AllowQualifiedName);
}

void LogTagMismatchCheck::registerMatchers(MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  // Match call expressions - we'll filter by macro name in check()
  Finder->addMatcher(callExpr().bind("logCall"), this);
}

const FunctionDecl* LogTagMismatchCheck::getEnclosingFunction(const Stmt* S, ASTContext* Ctx) {
  // Traverse up the parent chain to find the enclosing function
  const Stmt* Current = S;
  const FunctionDecl* FoundFunc = nullptr;

  while (Current) {
    // Get the parent of Current statement
    auto Parents = Ctx->getParents(*Current);
    if (Parents.empty()) {
      break;
    }

    const auto* ParentNode = Parents.begin();
    if (const auto* FD = ParentNode->get<FunctionDecl>()) {
      FoundFunc = FD;
      break;
    }
    if (const auto* MD = ParentNode->get<CXXMethodDecl>()) {
      FoundFunc = MD;
      break;
    }
    if (ParentNode->get<LambdaExpr>()) {
      // For lambdas, continue to find the outer function
      Current = Parents.begin()->get<Stmt>();
      continue;
    }
    if (const auto* ParentStmt = ParentNode->get<Stmt>()) {
      Current = ParentStmt;
    } else {
      break;
    }
  }

  return FoundFunc;
}

std::vector<std::string> LogTagMismatchCheck::extractTags(StringRef LogText) {
  std::vector<std::string> Tags;
  std::regex TagPattern(R"(\[([A-Za-z0-9_:]+)\])");
  std::string Text = LogText.str();

  auto WordsBegin = std::sregex_iterator(Text.begin(), Text.end(), TagPattern);
  auto WordsEnd = std::sregex_iterator();

  for (std::sregex_iterator I = WordsBegin; I != WordsEnd; ++I) {
    std::smatch Match = *I;
    Tags.push_back(Match[1].str());
  }

  return Tags;
}

bool LogTagMismatchCheck::isTagValid(StringRef Tag, const FunctionDecl* Func) {
  if (!Func) {
    return true; // Can't validate, assume valid
  }

  std::string FuncName = Func->getNameAsString();

  // Check direct match
  if (Tag == FuncName) {
    return true;
  }

  // Check qualified name match if enabled
  if (AllowQualifiedName) {
    if (const auto* MD = dyn_cast<CXXMethodDecl>(Func)) {
      std::string QualifiedName = MD->getParent()->getNameAsString() + "::" + FuncName;
      if (Tag == QualifiedName) {
        return true;
      }
    }
  }

  return false;
}

bool LogTagMismatchCheck::matchesMacroPattern(StringRef MacroName) {
  // Split LogMacroNames by commas and check each pattern
  std::string Patterns = LogMacroNames;
  size_t Pos = 0;
  while (Pos < Patterns.size()) {
    size_t Next = Patterns.find(',', Pos);
    std::string Pattern;
    if (Next == std::string::npos) {
      Pattern = Patterns.substr(Pos);
      Next = Patterns.size();
    } else {
      Pattern = Patterns.substr(Pos, Next - Pos);
    }
    Pos = Next + 1;

    // Simple glob matching: * = any chars
    // Convert to lowercase for case-insensitive match
    std::string MacroLower = MacroName.lower();
    std::string PatternLower;
    std::transform(Pattern.begin(), Pattern.end(), std::back_inserter(PatternLower),
                   [](unsigned char c) { return std::tolower(c); });

    // Simple pattern matching - strip * and check if contained
    std::string PatternStripped;
    for (char c : PatternLower) {
      if (c != '*') {
        PatternStripped += c;
      }
    }
    if (MacroLower.find(PatternStripped) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void LogTagMismatchCheck::check(const MatchFinder::MatchResult& Result) {
  if (Result.Context == nullptr) {
    return;
  }

  const auto* Call = Result.Nodes.getNodeAs<CallExpr>("logCall");
  if (Call == nullptr) {
    return;
  }

  // Check if this call is from a log macro
  SourceLocation CallLoc = Call->getBeginLoc();
  if (!CallLoc.isMacroID()) {
    return; // Not a macro call
  }

  SourceManager& SM = Result.Context->getSourceManager();
  StringRef MacroName = Lexer::getImmediateMacroName(CallLoc, SM, Result.Context->getLangOpts());

  if (!matchesMacroPattern(MacroName)) {
    return;
  }

  // Get enclosing function
  const FunctionDecl* EnclosingFunc = getEnclosingFunction(Call, Result.Context);
  if (!EnclosingFunc) {
    return;
  }

  std::string FuncName = EnclosingFunc->getNameAsString();

  // Check all arguments for string literals
  for (unsigned I = 0; I < Call->getNumArgs(); ++I) {
    const Expr* Arg = Call->getArg(I)->IgnoreImplicit();
    if (const auto* SL = dyn_cast<StringLiteral>(Arg)) {
      std::string LogText = SL->getString().str();
      std::vector<std::string> Tags = extractTags(LogText);

      for (const auto& Tag : Tags) {
        if (!isTagValid(Tag, EnclosingFunc)) {
          // Find location of the tag in the source
          SourceLocation TagLoc = SL->getBeginLoc();
          size_t TagPos = LogText.find("[" + Tag + "]");
          if (TagPos != std::string::npos) {
            TagLoc = TagLoc.getLocWithOffset(TagPos);
          }

          diag(TagLoc, "log tag '%0' does not match enclosing function '%1'")
              << Tag << FuncName
              << FixItHint::CreateReplacement(
                     SourceRange(TagLoc, TagLoc.getLocWithOffset(Tag.size() + 1)),
                     "[" + FuncName + "]");
        }
      }
    }
  }
}

} // namespace clang::tidy::codelint
