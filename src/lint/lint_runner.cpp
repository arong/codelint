#include "lint/lint_runner.h"
#include "lint/checkers/init_checker.h"
#include "lint/checkers/const_checker.h"
#include "lint/checkers/global_checker.h"
#include "lint/checkers/singleton_checker.h"

#include <rapidjson/document.h>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <set>
#include <map>

namespace codelint {
namespace lint {

LintRunner::LintRunner(const LintConfig& config)
    : config_(config) {
    init_checkers();
}

void LintRunner::init_checkers() {
    if (std::find(config_.enabled_checkers.begin(), config_.enabled_checkers.end(), "init") 
        != config_.enabled_checkers.end()) {
        checkers_.push_back(std::make_unique<InitChecker>());
    }
    if (std::find(config_.enabled_checkers.begin(), config_.enabled_checkers.end(), "const") 
        != config_.enabled_checkers.end()) {
        checkers_.push_back(std::make_unique<ConstChecker>());
    }
    if (std::find(config_.enabled_checkers.begin(), config_.enabled_checkers.end(), "global") 
        != config_.enabled_checkers.end()) {
        checkers_.push_back(std::make_unique<GlobalChecker>());
    }
    if (std::find(config_.enabled_checkers.begin(), config_.enabled_checkers.end(), "singleton") 
        != config_.enabled_checkers.end()) {
        checkers_.push_back(std::make_unique<SingletonChecker>());
    }
}

LintResult LintRunner::run(const std::vector<std::string>& files) {
    LintResult result;
    
    std::vector<std::string> file_list = files;
    if (file_list.empty()) {
        file_list = get_files_from_compile_commands(config_.path);
    }
    
    for (const auto& filepath : file_list) {
        if (!should_check_file(filepath)) {
            continue;
        }
        
        for (const auto& checker : checkers_) {
            LintResult checker_result = checker->check(filepath);
            result.merge(checker_result);
        }
    }
    
    return result;
}

LintResult LintRunner::run_checkers(const std::vector<std::string>& files,
                                    const std::vector<std::string>& checker_names) {
    LintResult result;
    
    std::vector<std::string> file_list = files;
    if (file_list.empty()) {
        file_list = get_files_from_compile_commands(config_.path);
    }
    
    for (const auto& filepath : file_list) {
        if (!should_check_file(filepath)) {
            continue;
        }
        
        for (const auto& checker : checkers_) {
            if (std::find(checker_names.begin(), checker_names.end(), checker->name()) 
                != checker_names.end()) {
                LintResult checker_result = checker->check(filepath);
                result.merge(checker_result);
            }
        }
    }
    
    return result;
}

std::vector<std::string> LintRunner::get_files_from_compile_commands(const std::string& path) const {
    std::vector<std::string> files;
    std::string compile_commands_path = path + "/compile_commands.json";
    
    std::ifstream file(compile_commands_path);
    if (!file.is_open()) {
        return files;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                       std::istreambuf_iterator<char>());
    
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        return files;
    }
    
    if (doc.IsArray()) {
        for (const auto& entry : doc.GetArray()) {
            if (entry.HasMember("file") && entry["file"].IsString()) {
                std::string filepath = entry["file"].GetString();
                // Only include C++ source files
                if (filepath.find(".cpp") != std::string::npos ||
                    filepath.find(".cc") != std::string::npos ||
                    filepath.find(".cxx") != std::string::npos ||
                    filepath.find(".hpp") != std::string::npos ||
                    filepath.find(".h") != std::string::npos) {
                    files.push_back(filepath);
                }
            }
        }
    }
    
    // Remove duplicates
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    
    return files;
}

bool LintRunner::should_check_file(const std::string& filepath) const {
    // Skip system headers
    if (filepath.find("/usr/include/") != std::string::npos ||
        filepath.find("/usr/lib/") != std::string::npos) {
        return false;
    }
    
    if (config_.exclude_patterns.empty() && config_.include_patterns.empty()) {
        return true;
    }
    
    bool should_include = config_.include_patterns.empty();
    for (const auto& pattern : config_.include_patterns) {
        if (filepath.find(pattern) != std::string::npos) {
            should_include = true;
            break;
        }
    }
    
    if (!should_include) {
        return false;
    }
    
    for (const auto& pattern : config_.exclude_patterns) {
        if (filepath.find(pattern) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

void LintRunner::print_human(const LintResult& result, std::ostream& os) const {
    if (result.issues.empty()) {
        os << "No issues found. Good job!\n";
        return;
    }
    
    os << "Found " << result.total_count() << " issue(s):\n";
    os << "  Errors: " << result.error_count << "\n";
    os << "  Warnings: " << result.warning_count << "\n";
    os << "  Info: " << result.info_count << "\n";
    os << "  Hints: " << result.hint_count << "\n\n";
    
    // Group by file
    std::map<std::string, std::vector<LintIssue>> file_issues;
    for (const auto& issue : result.issues) {
        file_issues[issue.file].push_back(issue);
    }
    
    for (const auto& [file, issues] : file_issues) {
        os << file << ":\n";
        for (const auto& issue : issues) {
            os << "  " << issue.line << ":" << issue.column 
               << " [" << issue.checker_name << "] ";
            
            switch (issue.severity) {
                case Severity::ERROR: os << "error: "; break;
                case Severity::WARNING: os << "warning: "; break;
                case Severity::INFO: os << "info: "; break;
                case Severity::HINT: os << "hint: "; break;
            }
            
            os << issue.description << "\n";
            if (!issue.suggestion.empty()) {
                os << "    Suggestion: " << issue.suggestion << "\n";
            }
            if (issue.fixable) {
                os << "    (auto-fixable)\n";
            }
        }
        os << "\n";
    }
}

void LintRunner::print_json(const LintResult& result, std::ostream& os) const {
    os << "{\n";
    os << "  \"total_issues\": " << result.total_count() << ",\n";
    os << "  \"errors\": " << result.error_count << ",\n";
    os << "  \"warnings\": " << result.warning_count << ",\n";
    os << "  \"info\": " << result.info_count << ",\n";
    os << "  \"hints\": " << result.hint_count << ",\n";
    os << "  \"fixed_count\": " << result.fixed_count << ",\n";
    os << "  \"issues\": [\n";
    
    for (size_t i = 0; i < result.issues.size(); ++i) {
        const auto& issue = result.issues[i];
        os << "    {\n";
        os << "      \"checker\": \"" << issue.checker_name << "\",\n";
        os << "      \"severity\": \"" << severity_to_string(issue.severity) << "\",\n";
        os << "      \"type\": \"" << check_type_to_string(issue.type) << "\",\n";
        os << "      \"name\": \"" << issue.name << "\",\n";
        os << "      \"type_str\": \"" << issue.type_str << "\",\n";
        os << "      \"file\": \"" << issue.file << "\",\n";
        os << "      \"line\": " << issue.line << ",\n";
        os << "      \"column\": " << issue.column << ",\n";
        os << "      \"description\": \"" << issue.description << "\",\n";
        os << "      \"suggestion\": \"" << issue.suggestion << "\",\n";
        os << "      \"fixable\": " << (issue.fixable ? "true" : "false") << "\n";
        os << "    }";
        if (i < result.issues.size() - 1) {
            os << ",";
        }
        os << "\n";
    }
    
    os << "  ]\n";
    os << "}\n";
}

int LintRunner::apply_fixes(const LintResult& result) {
    std::cerr << "Debug: apply_fixes called with " << result.issues.size() << " total issues" << std::endl;
    
    std::set<std::string> files_to_fix;
    
    for (const auto& issue : result.issues) {
        std::cerr << "Debug: Issue - fixable=" << issue.fixable << ", severity=" << (int)issue.severity << ", file=" << issue.file << std::endl;
        if (issue.fixable && issue.severity != Severity::ERROR) {
            files_to_fix.insert(issue.file);
        }
    }
    
    std::cerr << "Debug: Files to fix: " << files_to_fix.size() << std::endl;
    
    int fixed_count = 0;
    
    for (const auto& filepath : files_to_fix) {
        std::vector<LintIssue> file_issues;
        for (const auto& issue : result.issues) {
            if (issue.file == filepath && issue.fixable) {
                file_issues.push_back(issue);
            }
        }
        
        if (file_issues.empty()) continue;
        
        std::ifstream file(filepath);
        if (!file.is_open()) continue;
        
        std::string content((std::istreambuf_iterator<char>(file)), 
                           std::istreambuf_iterator<char>());
        file.close();
        
        std::string modified_content = content;
        
        for (const auto& checker : checkers_) {
            if (checker->can_fix()) {
                checker->apply_fixes(filepath, file_issues, modified_content);
            }
        }
        
        if (modified_content != content) {
            std::ofstream out_file(filepath);
            if (out_file.is_open()) {
                out_file << modified_content;
                out_file.close();
                fixed_count += file_issues.size();
            }
        }
    }
    
    return fixed_count;
}

LintConfig LintRunner::load_config(const std::string& config_path) {
    LintConfig config;
    
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return config;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                       std::istreambuf_iterator<char>());
    
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        return config;
    }
    
    if (doc.HasMember("lint") && doc["lint"].IsObject()) {
        const auto& lint = doc["lint"];
        
        if (lint.HasMember("enabled_checkers") && lint["enabled_checkers"].IsArray()) {
            config.enabled_checkers.clear();
            for (const auto& checker : lint["enabled_checkers"].GetArray()) {
                if (checker.IsString()) {
                    config.enabled_checkers.push_back(checker.GetString());
                }
            }
        }
        
        if (lint.HasMember("exclude_patterns") && lint["exclude_patterns"].IsArray()) {
            for (const auto& pattern : lint["exclude_patterns"].GetArray()) {
                if (pattern.IsString()) {
                    config.exclude_patterns.push_back(pattern.GetString());
                }
            }
        }
        
        if (lint.HasMember("min_severity") && lint["min_severity"].IsString()) {
            config.min_severity = string_to_severity(lint["min_severity"].GetString());
        }
        
        if (lint.HasMember("auto_fix") && lint["auto_fix"].IsBool()) {
            config.auto_fix = lint["auto_fix"].GetBool();
        }
        
        if (lint.HasMember("output_json") && lint["output_json"].IsBool()) {
            config.output_json = lint["output_json"].GetBool();
        }
    }
    
    return config;
}

}  // namespace lint
}  // namespace codelint
