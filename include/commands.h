#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lint/git_scope.h"
#include "lint/lint_types.h"

struct GlobalOptions {
  std::string path;
  bool output_json = false;
  bool output_sarif = false;
  bool show_version = false;
  std::string scope = "all";
};

// Options for check_init command
struct CheckInitOptions {
  std::vector<std::string> files;
  bool fix = false;
  bool inplace = false;
  bool suppress_constant = false;
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
extern std::optional<codelint::lint::GitScope> g_scope;

int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts);
int find_global(const GlobalOptions& opts, const FindGlobalOptions& global_opts);
int find_singleton(const GlobalOptions& opts, const FindSingletonOptions& singleton_opts);

// Utility functions
void format_output(const std::vector<codelint::lint::LintIssue>& issues, bool json_output,
                   bool sarif_output);
bool apply_fixes_to_file(const std::string& filepath,
                         const std::vector<codelint::lint::LintIssue>& fixes);
