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

// Options for check_init command
struct CheckInitOptions {
  std::vector<std::string> files;
  bool fix = false;
};

// Options for find_global command
struct FindGlobalOptions {
  std::string path;
};

// Options for find_singleton command
struct FindSingletonOptions {
  std::string path;
};

extern GlobalOptions g_opts;
extern codelint::lint::LintConfig g_lint_config;
extern std::optional<codelint::lint::GitScope> g_scope;

void lint();
int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts);
int find_global(const GlobalOptions& opts, const FindGlobalOptions& global_opts);
int find_singleton(const GlobalOptions& opts, const FindSingletonOptions& singleton_opts);

// Utility functions
void format_output(const std::vector<codelint::lint::LintIssue>& issues, bool json_output);
bool apply_fixes_to_file(const std::string& filepath, const std::vector<codelint::lint::LintIssue>& fixes);
