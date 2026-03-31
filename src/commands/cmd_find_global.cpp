#include "commands/cmd_find_global.h"

#include <functional>
#include <iostream>
#include <optional>

#include "commands/cmd_utils.h"
#include "lint/checkers/global_checker.h"
#include "lint/git_scope.h"
#include "lint/lint_types.h"

int find_global(const GlobalOptions& opts, const FindGlobalOptions& global_opts) {
  using namespace codelint::lint;

  std::optional<GitScope> scope;
  if (!opts.scope.empty() && opts.scope != "all") {
    scope = GitScope::parse(opts.scope);
  }

  return run_checker_command<GlobalChecker>(
      global_opts.path,
      opts.output_json,
      scope,
      "global variable",
      [](const std::optional<GitScope>& s) { return GlobalChecker(s); },
      [](const LintIssue& issue) {
        std::cout << "  " << issue.name << " : " << issue.type_str << "\n";
      }
  );
}
