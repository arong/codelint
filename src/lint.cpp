#include "commands.h"
#include "lint/lint_runner.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

void lint() {
    using namespace cndy::lint;
    
    LintConfig config = g_lint_config;
    
    // Set path from global options if not already set
    if (config.path == "." && !g_opts.path.empty()) {
        config.path = g_opts.path;
    }
    
    // Set output format
    config.output_json = g_opts.output_json;
    
    // Note: auto_fix and inplace are already set from g_lint_config via CLI11
    
    // Determine files to check
    std::vector<std::string> files;
    
    // If specific files are provided via command line, use them
    if (!config.files.empty()) {
        std::cerr << "Debug: Processing " << config.files.size() << " files" << std::endl;
        for (const auto& file : config.files) {
            std::cerr << "Debug: Checking file: " << file << std::endl;
            if (std::filesystem::exists(file)) {
                files.push_back(std::filesystem::canonical(file).string());
                std::cerr << "Debug: Added file: " << files.back() << std::endl;
            } else {
                std::cerr << "Error: File not found: " << file << std::endl;
            }
        }
    }
    
    if (files.empty() && config.files.empty()) {
        // Try to get files from compile_commands.json
        std::filesystem::path compile_commands_path = 
            std::filesystem::path(config.path) / "compile_commands.json";
        
        if (!std::filesystem::exists(compile_commands_path)) {
            std::cerr << "Error: No files specified and compile_commands.json not found in: " 
                      << config.path << std::endl;
            std::cerr << "Usage: cndy lint <file1.cpp> [file2.cpp] ..." << std::endl;
            return;
        }
    }
    
    // Create runner and execute
    LintRunner runner(config);
    LintResult result;
    
    // Filter checkers if --only is specified
    if (!config.only_checkers.empty()) {
        result = runner.run_checkers(files, config.only_checkers);
    } else {
        result = runner.run(files);
    }
    
    // Filter by minimum severity
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
    
    // Apply fixes if requested
    if (config.auto_fix && !result.issues.empty()) {
        int fixed = runner.apply_fixes(result);
        result.fixed_count = fixed;
        
        if (!config.output_json) {
            std::cout << "Applied " << fixed << " fix(es)." << std::endl;
        }
    }
    
    // Output results
    if (config.output_json) {
        runner.print_json(result, std::cout);
    } else {
        runner.print_human(result, std::cout);
    }
    
    // Return non-zero if there are errors
    if (result.error_count > 0) {
        // This would normally set an exit code, but since we're in a function,
        // the caller (main) would need to handle this
    }
}
