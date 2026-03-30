#pragma once

#include <string>
#include <vector>

#include "../lint/lint_types.h"

std::vector<std::string> collect_cpp_files(const std::string& path);

void format_output(const std::vector<codelint::lint::LintIssue>& issues, bool json_output);

bool apply_fixes_to_file(const std::string& filepath, const std::vector<codelint::lint::LintIssue>& fixes);
