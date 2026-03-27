#pragma once

#include <string>
#include <vector>

#include "lint/lint_types.h"

struct GlobalOptions {
  std::string path;
  bool output_json = false;
  bool fix = false;
  bool inplace = false;
};

struct CheckInitOptions {
  std::vector<std::string> files;
};

extern GlobalOptions g_opts;
extern CheckInitOptions g_check_init_opts;
extern codelint::lint::LintConfig g_lint_config;

void find_global();
void find_singleton();
void check_init();
void lint();