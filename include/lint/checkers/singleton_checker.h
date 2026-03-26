#pragma once

#include "../lint_checker.h"

namespace cndy {
namespace lint {

class SingletonChecker : public LintChecker {
public:
    LintResult check(const std::string& filepath) override;
    
    std::string name() const override { return "singleton"; }
    std::string description() const override { 
        return "Detect singleton patterns"; 
    }
    std::vector<CheckType> provides() const override {
        return {CheckType::SINGLETON_PATTERN};
    }
    
    bool can_fix() const override { return false; }

private:
    void visit_cursor(CXCursor cursor, LintResult& result);
};

}  // namespace lint
}  // namespace cndy