#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace cndy {
namespace lint {

enum class Severity {
    ERROR,
    WARNING,
    INFO,
    HINT
};

enum class CheckType {
    INIT_UNINITIALIZED,
    INIT_EQUALS_SYNTAX,
    INIT_UNSIGNED_SUFFIX,
    GLOBAL_VARIABLE,
    SINGLETON_PATTERN,
    CONST_SUGGESTION
};

struct LintIssue {
    CheckType type;
    Severity severity;
    std::string checker_name;   // "init", "global", "singleton", "const"
    std::string name;           // Variable/function name
    std::string type_str;       // Type string
    std::string file;
    int line;
    int column;
    std::string description;
    std::string suggestion;
    bool fixable = false;
};

struct LintResult {
    std::vector<LintIssue> issues;
    int error_count = 0;
    int warning_count = 0;
    int info_count = 0;
    int hint_count = 0;
    int fixed_count = 0;
    
    void add_issue(const LintIssue& issue) {
        issues.push_back(issue);
        switch (issue.severity) {
            case Severity::ERROR: ++error_count; break;
            case Severity::WARNING: ++warning_count; break;
            case Severity::INFO: ++info_count; break;
            case Severity::HINT: ++hint_count; break;
        }
    }
    
    void merge(const LintResult& other) {
        issues.insert(issues.end(), other.issues.begin(), other.issues.end());
        error_count += other.error_count;
        warning_count += other.warning_count;
        info_count += other.info_count;
        hint_count += other.hint_count;
        fixed_count += other.fixed_count;
    }
    
    int total_count() const {
        return error_count + warning_count + info_count + hint_count;
    }
};

// Config structures
struct LintConfig {
    std::vector<std::string> enabled_checkers = {"init", "global", "singleton", "const"};
    std::vector<std::string> only_checkers;
    std::vector<std::string> exclude_patterns;
    std::vector<std::string> include_patterns;
    std::vector<std::string> files;
    std::map<std::string, Severity> severity_overrides;
    bool auto_fix = false;
    bool inplace = false;
    bool output_json = false;
    Severity min_severity = Severity::INFO;
    std::string path = ".";
};

// Utility functions
inline std::string check_type_to_string(CheckType type) {
    switch (type) {
        case CheckType::INIT_UNINITIALIZED: return "init_uninitialized";
        case CheckType::INIT_EQUALS_SYNTAX: return "init_equals_syntax";
        case CheckType::INIT_UNSIGNED_SUFFIX: return "init_unsigned_suffix";
        case CheckType::GLOBAL_VARIABLE: return "global_variable";
        case CheckType::SINGLETON_PATTERN: return "singleton_pattern";
        case CheckType::CONST_SUGGESTION: return "const_suggestion";
        default: return "unknown";
    }
}

inline std::string severity_to_string(Severity sev) {
    switch (sev) {
        case Severity::ERROR: return "error";
        case Severity::WARNING: return "warning";
        case Severity::INFO: return "info";
        case Severity::HINT: return "hint";
        default: return "unknown";
    }
}

inline Severity string_to_severity(const std::string& str) {
    if (str == "error") return Severity::ERROR;
    if (str == "warning") return Severity::WARNING;
    if (str == "info") return Severity::INFO;
    if (str == "hint") return Severity::HINT;
    return Severity::INFO;
}

inline CheckType string_to_check_type(const std::string& str) {
    if (str == "init_uninitialized") return CheckType::INIT_UNINITIALIZED;
    if (str == "init_equals_syntax") return CheckType::INIT_EQUALS_SYNTAX;
    if (str == "init_unsigned_suffix") return CheckType::INIT_UNSIGNED_SUFFIX;
    if (str == "global_variable") return CheckType::GLOBAL_VARIABLE;
    if (str == "singleton_pattern") return CheckType::SINGLETON_PATTERN;
    if (str == "const_suggestion") return CheckType::CONST_SUGGESTION;
    return CheckType::INIT_UNINITIALIZED;  // default
}

}  // namespace lint
}  // namespace cndy