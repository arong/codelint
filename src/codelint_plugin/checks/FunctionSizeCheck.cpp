#include "codelint/checks/FunctionSizeCheck.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace clang::tidy::codelint {

void FunctionSizeCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(
      ast_matchers::functionDecl(ast_matchers::hasBody(ast_matchers::stmt())).bind("function"),
      this);
}

static bool isBlankLine(StringRef Line) {
  return Line.trim().empty();
}

static bool isCommentLine(StringRef Line) {
  return Line.find("//") != StringRef::npos;
}

static StringRef getLineText(const SourceManager& SrcMgr, FileID FID, unsigned LineNumber) {
  StringRef FullText = SrcMgr.getBufferData(FID);

  unsigned CurrentLine = 1;
  size_t LineStart = 0;

  for (size_t i = 0; i < FullText.size(); ++i) {
    if (CurrentLine == LineNumber) {
      LineStart = i;
      break;
    }
    if (FullText[i] == '\n') {
      ++CurrentLine;
    }
  }

  if (CurrentLine != LineNumber) {
    return StringRef();
  }

  size_t LineEnd = LineStart;
  while (LineEnd < FullText.size() && FullText[LineEnd] != '\n') {
    ++LineEnd;
  }

  return FullText.substr(LineStart, LineEnd - LineStart);
}

void FunctionSizeCheck::check(const ast_matchers::MatchFinder::MatchResult& Result) {
  if (Result.Context->getDiagnostics().hasErrorOccurred()) {
    return;
  }

  const auto* FunctionDecl = Result.Nodes.getNodeAs<clang::FunctionDecl>("function");
  if (FunctionDecl == nullptr) {
    return;
  }

  const auto* Body = FunctionDecl->getBody();
  if (Body == nullptr) {
    return;
  }

  const auto& SrcMgr = Result.Context->getSourceManager();

  const auto ExpansionStart = SrcMgr.getExpansionLoc(Body->getBeginLoc());
  const auto ExpansionEnd = SrcMgr.getExpansionLoc(Body->getEndLoc());

  if (!ExpansionStart.isValid() || !ExpansionEnd.isValid()) {
    return;
  }

  const auto FID = SrcMgr.getFileID(ExpansionStart);
  const auto StartLine = SrcMgr.getExpansionLineNumber(ExpansionStart);
  const auto EndLine = SrcMgr.getExpansionLineNumber(ExpansionEnd);

  int LineCount = 0;
  for (unsigned Line = StartLine; Line <= EndLine; ++Line) {
    StringRef LineText = getLineText(SrcMgr, FID, Line);

    if (isBlankLine(LineText) || isCommentLine(LineText)) {
      continue;
    }

    ++LineCount;
  }

  if (LineCount >= 50) {
    diag(FunctionDecl->getLocation(), "function '%0' has %1 lines (exceeds 50 line limit)")
        << FunctionDecl->getName() << LineCount;
  }
}

} // namespace clang::tidy::codelint
