#include "commands.h"
#include "lint/lint_runner.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

void lint() {
    using namespace codelint::lint;
    
    LintConfig config = g_lint_config;
    
    if (config.path == "." && !g_opts.path.empty()) {
        config.path = g_opts.path;
    }
    
    config.output_json = g_opts.output_json;
    
    std::vector<std::string> files;
    
    if (!config.files.empty()) {
        for (const auto& file : config.files) {
            if (std::filesystem::exists(file)) {
                files.push_back(std::filesystem::canonical(file).string());
            }
        }
    }
    
    if (files.empty() && config.files.empty()) {
        std::filesystem::path compile_commands_path = 
            std::filesystem::path(config.path) / "compile_commands.json";
        
        if (!std::filesystem::exists(compile_commands_path)) {
            std::cerr << "Error: No files specified and compile_commands.json not found in: " 
                      << config.path << std::endl;
            return;
        }
    }
    
    LintRunner runner(config);
    LintResult result;
    
    if (!config.only_checkers.empty()) {
        result = runner.run_checkers(files, config.only_checkers);
    } else {
        result = runner.run(files);
    }
    
    if (config.min_severity != Severity::INFO) {
        LintResult filtered_result;
        for (const auto& issue : result.issues) {
            int issue_level = static_cast<int>(issue.severity);
            int min_level = static_cast<int>(config.min_severity);
            if (issue_level <= min_level) {
                filtered_result.add_issue(issue);
            }
        }
        result = filtered_result;
    }
    
    if (config.auto_fix) {
        if (result.issues.empty()) {
            for (const auto& filepath : files) {
                std::ifstream file(filepath);
                if (file.is_open()) {
                    std::cout << file.rdbuf();
                    file.close();
                }
            }
        } else {
            for (const auto& filepath : files) {
                std::vector<LintIssue> file_issues;
                for (const auto& issue : result.issues) {
                    if (issue.file == filepath && issue.fixable) {
                        file_issues.push_back(issue);
                    }
                }
                
                if (file_issues.empty()) {
                    std::ifstream file(filepath);
                    if (file.is_open()) {
                        std::cout << file.rdbuf();
                        file.close();
                    }
                    continue;
                }
                
                std::ifstream file(filepath);
                if (!file.is_open()) continue;
                
                std::string content((std::istreambuf_iterator<char>(file)), 
                                   std::istreambuf_iterator<char>());
                file.close();
                
                std::string modified_content = content;
                
                for (const auto& checker : runner.checkers()) {
                    if (checker->can_fix()) {
                        checker->apply_fixes(filepath, file_issues, modified_content);
                    }
                }
                
                if (config.inplace) {
                    std::ofstream out_file(filepath);
                    if (out_file.is_open()) {
                        out_file << modified_content;
                        out_file.close();
                    }
                } else {
                    std::cout << modified_content;
                }
            }
        }
    } else {
        if (config.output_json) {
            runner.print_json(result, std::cout);
        } else {
            runner.print_human(result, std::cout);
        }
    }
}
