#include "commands/cmd_find_singleton.h"

#include <functional>
#include <iostream>
#include <optional>

#include "commands/cmd_utils.h"
#include "lint/checkers/singleton_checker.h"
#include "lint/git_scope.h"
#include "lint/lint_types.h"

int find_singleton(const GlobalOptions& opts, const FindSingletonOptions& singleton_opts) {
  using namespace codelint::lint;

  std::optional<GitScope> scope;
  if (!opts.scope.empty() && opts.scope != "all") {
    scope = GitScope::parse(opts.scope);
  }

  return run_checker_command<SingletonChecker>(
      singleton_opts.path,
      opts.output_json,
      scope,
      "singleton pattern",
      [](const std::optional<GitScope>& s) { return SingletonChecker(s); },
      [](const LintIssue& issue) {
        std::cout << "  " << issue.suggestion << "\n";
      }
  );
}
