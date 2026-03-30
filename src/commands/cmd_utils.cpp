#include "commands/cmd_utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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
