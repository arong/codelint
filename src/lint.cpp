#include "commands.h"
#include "lint/lint_runner.h"
#include "lint/git_scope.h"

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
    config.scope = g_opts.scope;
    
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
    
    // Parse scope if not "all"
    std::optional<GitScope> git_scope;
    if (config.scope != "all") {
        git_scope = GitScope::parse(config.scope);
        if (!git_scope) {
            std::cerr << "Error: Failed to parse scope '" << config.scope << "'" << std::endl;
            return;
        }
        if (git_scope->has_error()) {
            std::cerr << "Error: " << git_scope->error() << std::endl;
            return;
        }
        
        // Filter files to only those in scope
        std::vector<std::string> scope_files = git_scope->get_modified_files();
        if (!scope_files.empty()) {
            // Intersection: keep only files that are in both lists
            std::vector<std::string> filtered_files;
            for (const auto& file : files) {
                if (std::find(scope_files.begin(), scope_files.end(), file) != scope_files.end()) {
                    filtered_files.push_back(file);
                }
            }
            files = filtered_files;
        }
        
        if (files.empty()) {
            std::cout << "No files to check in scope '" << config.scope << "'" << std::endl;
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
    
    if (git_scope) {
        LintResult filtered_result;
        for (const auto& issue : result.issues) {
            if (git_scope->is_line_modified(issue.file, issue.line)) {
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
