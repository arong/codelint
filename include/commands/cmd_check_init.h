#pragma once

#include "commands.h"

namespace codelint {
namespace lint {
class InitChecker;
class ConstChecker;
class GitScope;
class LintIssue;
} // namespace lint

std::vector<lint::LintIssue> collect_issues(const std::vector<std::string>& paths,
                                            const std::optional<lint::GitScope>& scope,
                                            bool suppress_constant);

int apply_fixes(const std::vector<lint::LintIssue>& issues, bool inplace);

int format_output(const std::vector<lint::LintIssue>& issues, bool output_json, bool output_sarif = false);

} // namespace codelint

int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts);
