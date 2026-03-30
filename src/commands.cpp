#include "commands.h"
#include "lint/lint_checker.h"
#include "lint/lint_types.h"
#include "lint/git_scope.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace {

// Print a single issue
void print_issue(const codelint::lint::LintIssue& issue) {
    std::cout << issue.file << ":" << issue.line << ":" << issue.column << ": "
              << codelint::lint::severity_to_string(issue.severity) << ": "
              << issue.description << " [" << issue.checker_name << "]\n";
    std::cout << "  " << issue.suggestion << "\n";
}

// Print summary
void print_summary(const codelint::lint::LintResult& result) {
    if (result.error_count > 0) {
        std::cout << result.error_count << " error(s)";
    }
    if (result.warning_count > 0) {
        if (result.error_count > 0) std::cout << ", ";
        std::cout << result.warning_count << " warning(s)";
    }
    if (result.info_count > 0) {
        if (result.error_count + result.warning_count > 0) std::cout << ", ";
        std::cout << result.info_count << " info(s)";
    }
    if (result.hint_count > 0) {
        if (result.error_count + result.warning_count + result.info_count > 0) std::cout << ", ";
        std::cout << result.hint_count << " hint(s)";
    }
    std::cout << "\n";
}

}  // namespace

void lint() {
    using namespace codelint::lint;

    // Get files to lint
    std::vector<std::string> files = g_lint_config.files;
    if (files.empty()) {
        std::cerr << "Error: No files specified\n";
        return;
    }

    // Determine which checkers to run
    std::vector<std::string> checker_names;
    if (!g_lint_config.only_checkers.empty()) {
        checker_names = g_lint_config.only_checkers;
    } else {
        checker_names = CheckerFactory::available_checkers();
    }

    // Check for excluded checkers
    std::vector<std::string> active_checkers;
    for (const auto& name : checker_names) {
        bool excluded = false;
        for (const auto& excl : g_lint_config.exclude_patterns) {
            if (name == excl) {
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            active_checkers.push_back(name);
        }
    }

    // Create checkers with scope
    std::vector<std::unique_ptr<LintChecker>> checkers;
    for (const auto& name : active_checkers) {
        auto checker = CheckerFactory::create(name, g_scope);
        if (checker) {
            checkers.push_back(std::move(checker));
        }
    }

    if (checkers.empty()) {
        std::cerr << "Error: No active checkers\n";
        return;
    }

    // Run checkers on each file
    LintResult result;

    for (const auto& filepath : files) {
        // If scope is set, check if file should be processed
        if (g_scope.has_value()) {
            auto modified_files = g_scope->get_modified_files();
            bool is_modified = false;
            for (const auto& mf : modified_files) {
                if (mf == filepath || filepath.find(mf) != std::string::npos) {
                    is_modified = true;
                    break;
                }
            }

            // If scope is "all", skip this check
            if (g_opts.scope == "all") {
                // Process all files
            } else if (!is_modified && !modified_files.empty()) {
                std::cout << "Skipping unmodified file: " << filepath << "\n";
                continue;
            }
        }

        // Run each checker on this file
        for (auto& checker : checkers) {
            std::cout << "Running " << checker->name() << " on " << filepath << "...\n";
            auto checker_result = checker->check(filepath);

            // Filter by severity
            for (const auto& issue : checker_result.issues) {
                if (issue.severity >= g_lint_config.min_severity) {
                    result.add_issue(issue);
                }
            }
        }
    }

    // Print results
    std::cout << "\n=== Results ===\n";

    if (g_lint_config.auto_fix && result.fixed_count > 0) {
        const std::string& filepath = files.front();
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filepath << "\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string original_content = buffer.str();
        file.close();

        std::string modified_content = original_content;
        for (auto& checker : checkers) {
            if (checker->can_fix()) {
                std::vector<LintIssue> fixable_issues;
                for (const auto& issue : result.issues) {
                    if (issue.checker_name == checker->name() && issue.fixable) {
                        fixable_issues.push_back(issue);
                    }
                }
                if (!fixable_issues.empty()) {
                    checker->apply_fixes(filepath, fixable_issues, modified_content);
                }
            }
        }

        std::cout << modified_content;
    } else {
        for (const auto& issue : result.issues) {
            print_issue(issue);
        }
        print_summary(result);
    }
}
