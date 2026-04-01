#include "commands/cmd_utils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "lint/checkers/global_checker.h"
#include "lint/checkers/singleton_checker.h"
#include "lint/fix_applier.h"
#include "lint/issue_reporter.h"

std::vector<std::string> collect_cpp_files(const std::string& path) {
    std::vector<std::string> files;
    std::filesystem::path p(path);

    if (std::filesystem::is_regular_file(p)) {
        files.push_back(std::filesystem::absolute(p).string());
        return files;
    }

    if (!std::filesystem::is_directory(p)) {
        std::cerr << "Error: Path does not exist: " << path << "\n";
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(p)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string ext = entry.path().extension().string();
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" ||
            ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".C") {
            files.push_back(std::filesystem::absolute(entry.path()).string());
        }
    }

    return files;
}

void format_output(const std::vector<codelint::lint::LintIssue>& issues, bool json_output) {
    codelint::lint::IssueReporter reporter;
    reporter.add_issues(issues);

    if (json_output) {
        reporter.print_json();
    } else {
        reporter.print_text();
    }
}

void print_statistics(int files_processed, int issues_found, std::chrono::milliseconds elapsed) {
    std::cout << "Statistics:\n";
    std::cout << "  Files processed: " << files_processed << "\n";
    std::cout << "  Issues found: " << issues_found << "\n";
    std::cout << "  Time elapsed: " << elapsed.count() << "ms\n";
}

bool apply_fixes_to_file(const std::string& filepath,
                         const std::vector<codelint::lint::LintIssue>& fixes) {
    if (fixes.empty()) {
        return true;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filepath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    codelint::lint::FixApplier applier;
    std::string modified_content;
    bool success = applier.applyFixes(fixes, content, modified_content, filepath);

    if (!success || modified_content.empty()) {
        std::cerr << "Error: Failed to apply fixes to " << filepath << "\n";
        return false;
    }

    std::ofstream out_file(filepath);
    if (!out_file.is_open()) {
        std::cerr << "Error: Cannot write to file " << filepath << "\n";
        return false;
    }

    out_file << modified_content;
    out_file.close();

    std::cout << "Applied " << applier.getAppliedFixCount()
              << " fix(es) to " << filepath << "\n";

    return true;
}

template<typename CheckerType>
int run_checker_command(
    const std::string& path,
    bool output_json,
    const std::optional<codelint::lint::GitScope>& scope,
    const std::string& result_noun,
    std::function<CheckerType(const std::optional<codelint::lint::GitScope>&)> checker_factory,
    std::function<void(const codelint::lint::LintIssue&)> print_issue_extra
) {
    using namespace codelint::lint;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::string> files = collect_cpp_files(path);

    if (files.empty()) {
        if (output_json) {
            std::cout << "{\"issues\": []}\n";
        } else {
            std::cerr << "Error: No C++ files found in " << path << std::endl;
        }
        return 1;
    }

    CheckerType checker = checker_factory(scope);
    std::vector<LintIssue> all_issues;

    for (const auto& filepath : files) {
        if (scope.has_value()) {
            auto modified_files = scope->get_modified_files();
            if (!modified_files.empty()) {
                bool should_skip = true;
                for (const auto& mf : modified_files) {
                    if (mf == filepath || filepath.find(mf) != std::string::npos) {
                        should_skip = false;
                        break;
                    }
                }
                if (should_skip) {
                    std::cout << "Skipping unmodified file: " << filepath << "\n";
                    continue;
                }
            }
        }

        if (!output_json) {
            std::cout << "Checking " << filepath << "...\n";
        }

        LintResult result = checker.check(filepath);
        all_issues.insert(all_issues.end(), result.issues.begin(), result.issues.end());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    int files_processed = static_cast<int>(files.size());

    if (output_json) {
        std::cout << "{\n";
        std::cout << "  \"issues\": [\n";
        for (size_t i = 0; i < all_issues.size(); ++i) {
            const auto& issue = all_issues[i];
            std::cout << "    {\n";
            std::cout << R"(      "type": ")" << check_type_to_string(issue.type) << "\",\n";
            std::cout << R"(      "severity": ")" << severity_to_string(issue.severity) << "\",\n";
            std::cout << R"(      "checker": ")" << issue.checker_name << "\",\n";
            std::cout << R"(      "name": ")" << issue.name << "\",\n";
            std::cout << "      \"type_str\": \"" << issue.type_str << "\",\n";
            std::cout << "      \"file\": \"" << issue.file << "\",\n";
            std::cout << "      \"line\": " << issue.line << ",\n";
            std::cout << "      \"column\": " << issue.column << ",\n";
            std::cout << "      \"description\": \"" << issue.description << "\",\n";
            std::cout << "      \"suggestion\": \"" << issue.suggestion << "\",\n";
            std::cout << "      \"fixable\": " << (issue.fixable ? "true" : "false") << "\n";
            std::cout << "    }";
            if (i < all_issues.size() - 1) {
                std::cout << ",";
            }
            std::cout << "\n";
        }
        std::cout << "  ]\n";
        std::cout << "}\n";
    } else {
        for (const auto& issue : all_issues) {
            std::cout << issue.file << ":" << issue.line << ":" << issue.column << ": "
                      << severity_to_string(issue.severity) << ": "
                      << issue.description << " [" << issue.checker_name << "]\n";
            if (print_issue_extra) {
                print_issue_extra(issue);
            }
        }

        int count = static_cast<int>(all_issues.size());
        if (count > 0) {
            std::cout << "Found " << count << " " << result_noun << "(s)\n";
        } else {
            std::cout << "No " << result_noun << "s found\n";
        }

        print_statistics(files_processed, static_cast<int>(all_issues.size()), elapsed_ms);
    }

    return 0;
}

template int run_checker_command<codelint::lint::GlobalChecker>(
    const std::string& path,
    bool output_json,
    const std::optional<codelint::lint::GitScope>& scope,
    const std::string& result_noun,
    std::function<codelint::lint::GlobalChecker(const std::optional<codelint::lint::GitScope>&)> checker_factory,
    std::function<void(const codelint::lint::LintIssue&)> print_issue_extra
);

template int run_checker_command<codelint::lint::SingletonChecker>(
    const std::string& path,
    bool output_json,
    const std::optional<codelint::lint::GitScope>& scope,
    const std::string& result_noun,
    std::function<codelint::lint::SingletonChecker(const std::optional<codelint::lint::GitScope>&)> checker_factory,
    std::function<void(const codelint::lint::LintIssue&)> print_issue_extra
);
