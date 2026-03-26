#pragma once

#include "lint_checker.h"
#include "lint_types.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace cndy {
namespace lint {

class LintRunner {
public:
    explicit LintRunner(const LintConfig& config);
    
    static LintConfig load_config(const std::string& config_path);
    
    LintResult run(const std::vector<std::string>& files);
    
    LintResult run_checkers(const std::vector<std::string>& files,
                           const std::vector<std::string>& checker_names);
    
    void print_human(const LintResult& result, std::ostream& os = std::cout) const;
    void print_json(const LintResult& result, std::ostream& os = std::cout) const;
    
    int apply_fixes(const LintResult& result);
    
private:
    LintConfig config_;
    std::vector<std::unique_ptr<LintChecker>> checkers_;
    
    void init_checkers();
    bool should_check_file(const std::string& filepath) const;
    std::vector<std::string> get_files_from_compile_commands(const std::string& path) const;
};

}  // namespace lint
}  // namespace cndy