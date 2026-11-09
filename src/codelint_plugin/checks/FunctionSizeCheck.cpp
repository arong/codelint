#include "codelint/checks/FunctionSizeCheck.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

namespace clang::tidy::codelint {

void FunctionSizeCheck::registerMatchers(ast_matchers::MatchFinder* Finder) {
  if (Finder == nullptr) {
    return;
  }

  Finder->addMatcher(
      ast_matchers::functionDecl(ast_matchers::hasBody(ast_matchers::stmt())).bind("function"),
      this);
}

static bool isBlankOrLeadingCommentLine(StringRef Line) {
  const StringRef Trimmed = Line.trim();
  if (Trimmed.empty()) {
    return true;
  }
  return Trimmed.size() >= 2 && Trimmed[0] == '/' && Trimmed[1] == '/';
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

  const auto StartDecomp = SrcMgr.getDecomposedExpansionLoc(ExpansionStart);
  const auto EndDecomp = SrcMgr.getDecomposedExpansionLoc(ExpansionEnd);
  if (StartDecomp.first != EndDecomp.first) {
    return;
  }

  const FileID FID = StartDecomp.first;
  const StringRef Buffer = SrcMgr.getBufferData(FID);
  if (StartDecomp.second >= Buffer.size() || EndDecomp.second >= Buffer.size()) {
    return;
  }

  // Single pass over the function body byte range.
  int LineCount = 0;
  size_t LineStart = StartDecomp.second;
  const size_t BodyEnd = EndDecomp.second;
  for (size_t I = StartDecomp.second; I <= BodyEnd; ++I) {
    const bool AtEnd = (I == BodyEnd);
    if (!AtEnd && Buffer[I] != '\n') {
      continue;
    }

    const size_t LineEnd = AtEnd && Buffer[I] != '\n' ? I + 1 : I;
    const StringRef LineText = Buffer.substr(LineStart, LineEnd - LineStart);
    if (!isBlankOrLeadingCommentLine(LineText)) {
      ++LineCount;
    }

    LineStart = I + 1;
    if (AtEnd) {
      break;
    }
  }

  if (LineCount > 50) {
    diag(FunctionDecl->getLocation(), "function '%0' has %1 lines (exceeds 50 line limit)")
        << FunctionDecl->getName() << LineCount;
  }
}

} // namespace clang::tidy::codelint
