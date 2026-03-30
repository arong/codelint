#include "commands/cmd_check_init.h"

#include "lint/checkers/init_checker.h"
#include "lint/checkers/const_checker.h"
#include "lint/fix_applier.h"
#include "lint/git_scope.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>

namespace {

bool is_cpp_file(const std::string& path) {
    return path.ends_with(".cpp") || path.ends_with(".cc") ||
           path.ends_with(".cxx") || path.ends_with(".c++") ||
           path.ends_with(".h") || path.ends_with(".hpp") ||
           path.ends_with(".hh") || path.ends_with(".hxx");
}

void collect_files_recursive(const std::string& path, std::vector<std::string>& files) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return;
    }

    if (S_ISREG(st.st_mode)) {
        if (is_cpp_file(path)) {
            files.push_back(path);
        }
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") {
                continue;
            }
            std::string full_path = path + "/" + name;
            collect_files_recursive(full_path, files);
        }
        closedir(dir);
    }
}

std::string read_file_content(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_file_content(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    return true;
}

bool should_skip_file(const std::string& filepath,
                      const std::optional<codelint::lint::GitScope>& scope) {
    if (!scope.has_value()) {
        return false;
    }

    if (scope->get_modified_files().empty()) {
        return false;
    }

    auto modified_files = scope->get_modified_files();
    for (const auto& mf : modified_files) {
        if (filepath == mf || filepath.find(mf) != std::string::npos) {
            return false;
        }
    }

    return true;
}

}  // namespace

int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts) {
    using namespace codelint::lint;

    std::vector<std::string> files;
    for (const auto& path : init_opts.files) {
        collect_files_recursive(path, files);
    }

    if (files.empty()) {
        std::cerr << "Error: No C++ files found in provided paths\n";
        return 1;
    }

    std::optional<GitScope> scope;
    if (!opts.scope.empty() && opts.scope != "all") {
        scope = GitScope::parse(opts.scope);
    }

    InitChecker init_checker(scope);
    ConstChecker const_checker(scope);

    std::vector<LintIssue> all_issues;

    for (const auto& filepath : files) {
        if (should_skip_file(filepath, scope)) {
            std::cout << "Skipping unmodified file: " << filepath << "\n";
            continue;
        }

        auto init_result = init_checker.check(filepath);
        for (const auto& issue : init_result.issues) {
            all_issues.push_back(issue);
        }

        if (!init_opts.suppress_constant) {
            auto const_result = const_checker.check(filepath);
            for (const auto& issue : const_result.issues) {
                all_issues.push_back(issue);
            }
        }
    }

    int total_issues = all_issues.size();
    int fixable_count = 0;
    for (const auto& issue : all_issues) {
        if (issue.fixable) {
            ++fixable_count;
        }
    }

    if (init_opts.fix && fixable_count > 0) {
        FixApplier fix_applier;
        std::map<std::string, std::vector<LintIssue>> issues_by_file;

        for (const auto& issue : all_issues) {
            if (issue.fixable) {
                issues_by_file[issue.file].push_back(issue);
            }
        }

        for (const auto& [filepath, file_issues] : issues_by_file) {
            std::string original_content = read_file_content(filepath);
            if (original_content.empty()) {
                std::cerr << "Warning: Cannot read file " << filepath << "\n";
                continue;
            }

            std::string modified_content = original_content;
            if (fix_applier.applyFixes(file_issues, original_content, modified_content, filepath)) {
                if (init_opts.inplace) {
                    if (write_file_content(filepath, modified_content)) {
                        std::cout << "Applied " << fix_applier.getAppliedFixCount()
                                  << " fix(es) to " << filepath << "\n";
                    } else {
                        std::cerr << "Error: Cannot write to file " << filepath << "\n";
                    }
                } else {
                    std::cout << modified_content;
                }
            }
        }

        return 0;
    }

    if (opts.output_json) {
        using namespace rapidjson;

        Document doc;
        doc.SetObject();

        Value issues_array(kArrayType);
        for (const auto& issue : all_issues) {
            Value issue_obj(kObjectType);

            Value type_val;
            type_val.SetString(check_type_to_string(issue.type).c_str(), doc.GetAllocator());
            issue_obj.AddMember("type", type_val, doc.GetAllocator());

            Value sev_val;
            sev_val.SetString(severity_to_string(issue.severity).c_str(), doc.GetAllocator());
            issue_obj.AddMember("severity", sev_val, doc.GetAllocator());

            Value checker_val;
            checker_val.SetString(issue.checker_name.c_str(), doc.GetAllocator());
            issue_obj.AddMember("checker", checker_val, doc.GetAllocator());

            Value name_val;
            name_val.SetString(issue.name.c_str(), doc.GetAllocator());
            issue_obj.AddMember("name", name_val, doc.GetAllocator());

            Value type_str_val;
            type_str_val.SetString(issue.type_str.c_str(), doc.GetAllocator());
            issue_obj.AddMember("type_str", type_str_val, doc.GetAllocator());

            Value file_val;
            file_val.SetString(issue.file.c_str(), doc.GetAllocator());
            issue_obj.AddMember("file", file_val, doc.GetAllocator());

            issue_obj.AddMember("line", issue.line, doc.GetAllocator());
            issue_obj.AddMember("column", issue.column, doc.GetAllocator());

            Value desc_val;
            desc_val.SetString(issue.description.c_str(), doc.GetAllocator());
            issue_obj.AddMember("description", desc_val, doc.GetAllocator());

            Value sugg_val;
            sugg_val.SetString(issue.suggestion.c_str(), doc.GetAllocator());
            issue_obj.AddMember("suggestion", sugg_val, doc.GetAllocator());

            issue_obj.AddMember("fixable", issue.fixable, doc.GetAllocator());

            issues_array.PushBack(issue_obj, doc.GetAllocator());
        }

        doc.AddMember("issues", issues_array, doc.GetAllocator());

        Value summary(kObjectType);
        int error_count = 0, warning_count = 0, info_count = 0, hint_count = 0;
        for (const auto& issue : all_issues) {
            switch (issue.severity) {
                case Severity::ERROR: ++error_count; break;
                case Severity::WARNING: ++warning_count; break;
                case Severity::INFO: ++info_count; break;
                case Severity::HINT: ++hint_count; break;
            }
        }
        summary.AddMember("errors", error_count, doc.GetAllocator());
        summary.AddMember("warnings", warning_count, doc.GetAllocator());
        summary.AddMember("info", info_count, doc.GetAllocator());
        summary.AddMember("hints", hint_count, doc.GetAllocator());
        summary.AddMember("total", total_issues, doc.GetAllocator());
        summary.AddMember("fixable", fixable_count, doc.GetAllocator());

        doc.AddMember("summary", summary, doc.GetAllocator());

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::cout << buffer.GetString() << std::endl;
    } else {
        if (all_issues.empty()) {
            std::cout << "No issues found.\n";
            return 0;
        }

        for (const auto& issue : all_issues) {
            std::cout << issue.file << ":" << issue.line << ":" << issue.column << ": "
                      << severity_to_string(issue.severity) << ": "
                      << issue.description << " [" << issue.checker_name << "]\n";
            if (!issue.suggestion.empty()) {
                std::cout << "  suggestion: " << issue.suggestion << "\n";
            }
        }

        std::cout << "\n";
        int error_count = 0, warning_count = 0, info_count = 0, hint_count = 0;
        for (const auto& issue : all_issues) {
            switch (issue.severity) {
                case Severity::ERROR: ++error_count; break;
                case Severity::WARNING: ++warning_count; break;
                case Severity::INFO: ++info_count; break;
                case Severity::HINT: ++hint_count; break;
            }
        }

        if (error_count > 0) {
            std::cout << error_count << " error(s)";
        }
        if (warning_count > 0) {
            if (error_count > 0) std::cout << ", ";
            std::cout << warning_count << " warning(s)";
        }
        if (info_count > 0) {
            if (error_count + warning_count > 0) std::cout << ", ";
            std::cout << info_count << " info(s)";
        }
        if (hint_count > 0) {
            if (error_count + warning_count + info_count > 0) std::cout << ", ";
            std::cout << hint_count << " hint(s)";
        }
        if (fixable_count > 0) {
            std::cout << ", " << fixable_count << " fixable";
        }
        std::cout << "\n";
    }

    return total_issues > 0 ? 1 : 0;
}
