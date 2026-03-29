#pragma once

#include <string>
#include <optional>

#include "lint/lint_types.h"
#include "lint/git_scope.h"

struct GlobalOptions {
  std::string path;
  bool output_json = false;
  bool fix = false;
  bool inplace = false;
  std::string scope = "all";
};

extern GlobalOptions g_opts;
extern codelint::lint::LintConfig g_lint_config;
extern std::optional<codelint::lint::GitScope> g_scope;

void lint();
