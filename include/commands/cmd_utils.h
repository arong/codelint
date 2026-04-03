#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "../lint/lint_checker.h"
#include "../lint/lint_types.h"

std::vector<std::string> collect_cpp_files(const std::string& path);

void format_output(const std::vector<codelint::lint::LintIssue>& issues, bool json_output);

void print_statistics(int files_processed, int issues_found, std::chrono::milliseconds elapsed);

bool apply_fixes_to_file(const std::string& filepath,
                         const std::vector<codelint::lint::LintIssue>& fixes);

template <typename CheckerType>
int run_checker_command(
    const std::string& path, bool output_json, const std::optional<codelint::lint::GitScope>& scope,
    const std::string& result_noun,
    std::function<CheckerType(const std::optional<codelint::lint::GitScope>&)> checker_factory,
    std::function<void(const codelint::lint::LintIssue&)> print_issue_extra = nullptr);
