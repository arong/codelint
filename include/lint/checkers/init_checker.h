#pragma once

#include "../lint_checker.h"

namespace cndy {
namespace lint {

class InitChecker : public LintChecker {
public:
    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "init"; }
    std::string description() const override { 
        return "Check variable initialization style"; 
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::INIT_UNINITIALIZED, 
                CheckType::INIT_EQUALS_SYNTAX, 
                CheckType::INIT_UNSIGNED_SUFFIX};
    }
    
    bool can_fix() const override { return true; }
    bool apply_fixes(const std::string& filepath,
                    const std::vector<LintIssue>& issues,
                    std::string& modified_content) override;

private:
    void visit_translation_unit(CXCursor cursor, LintResult& result);
    void visit_function_body(CXCursor cursor, LintResult& result);
    void check_var_decl(CXCursor cursor, LintResult& result);
    void check_uninitialized(CXCursor cursor, const std::string& var_name,
                            const std::string& type_str, LintResult& result);
    void check_equals_init(CXCursor cursor, CXCursor init_cursor, 
                          const std::string& var_name,
                          const std::string& type_str, LintResult& result);
    void check_unsigned_suffix(CXCursor cursor, const std::string& var_name,
                               const std::string& type_str, LintResult& result);
    
    bool is_unsigned_type(const std::string& type_str) const;
    bool has_digit_value(const std::string& value) const;
    bool has_unsigned_suffix(const std::string& value) const;
};

}  // namespace lint
}  // namespace cndy