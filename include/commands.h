#pragma once

#include <string>

#include "lint/lint_types.h"

struct GlobalOptions {
  std::string path;
  bool output_json = false;
  bool fix = false;
  bool inplace = false;
  std::string scope = "all";
};

extern GlobalOptions g_opts;
extern codelint::lint::LintConfig g_lint_config;

void lint();
