#pragma once

#include "../lint_checker.h"

namespace codelint {
namespace lint {

class GlobalChecker : public LintChecker {
public:
    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "global"; }
    std::string description() const override { 
        return "Detect global variables"; 
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::GLOBAL_VARIABLE};
    }
    
    bool can_fix() const override { return false; }

private:
    void visit_cursor(CXCursor cursor, LintResult& result);
};

}  // namespace lint
}  // namespace codelint