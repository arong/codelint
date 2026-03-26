#pragma once

#include "../lint_checker.h"
#include <unordered_map>
#include <unordered_set>

namespace cndy {
namespace lint {

class ConstChecker : public LintChecker {
public:
    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "const"; }
    std::string description() const override {
        return "Suggest const/constexpr for variables that don't change";
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::CONST_SUGGESTION};
    }
    
    bool can_fix() const override { return true; }
    bool apply_fixes(const std::string& filepath,
                     const std::vector<LintIssue>& issues,
                     std::string& modified_content) override;

    void collect_variables(CXCursor cursor, const std::string& filepath, LintResult& result);
    void track_modifications(CXCursor cursor);
    void visit_cursor(CXCursor cursor, const std::string& filepath, LintResult& result);

private:
    struct VarInfo {
        std::string name;
        std::string type;
        std::string file;
        int line;
        int column;
        bool is_assigned = false;
        bool is_modified = false;
        bool is_const = false;
        bool is_constexpr = false;
        bool is_reference = false;
        bool is_pointer = false;
        bool is_member = false;
        bool is_global = false;
        bool has_constexpr_init = false;
    };
    
    std::unordered_map<std::string, VarInfo> variables_;
    std::unordered_set<std::string> modified_vars_;
    
    void analyze_and_report(const std::string& filepath, LintResult& result);
    bool is_builtin_type(const std::string& type) const;
    bool can_be_constexpr(const VarInfo& info) const;
    std::string make_const_suggestion(const VarInfo& info) const;
};

}  // namespace lint
}  // namespace cndy