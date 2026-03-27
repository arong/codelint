#pragma once

#include <string>
#include <vector>

#include "lint_checker.h"

namespace codelint {
namespace lint {

class InitChecker : public LintChecker {
public:
    InitChecker();
    std::string name() const override;
    std::vector<LintIssue> check(const std::string& filepath) override;
    bool canFix() const override { return true; }
    bool fix(const LintIssue& issue) override;

private:
    struct Issue {
        std::string name;
        std::string type_str;
        std::string file;
        int line;
        std::string suggestion;
    };

    std::vector<Issue> find_issues(const std::string& filepath);
    std::string fix_issue(const std::string& content, const Issue& issue);
};

class GlobalVarChecker : public LintChecker {
public:
    GlobalVarChecker();
    std::string name() const override;
    std::vector<LintIssue> check(const std::string& filepath) override;
};

class SingletonChecker : public LintChecker {
public:
    SingletonChecker();
    std::string name() const override;
    std::vector<LintIssue> check(const std::string& filepath) override;
};

class ConstChecker : public LintChecker {
public:
    ConstChecker();
    std::string name() const override;
    std::vector<LintIssue> check(const std::string& filepath) override;
    bool canFix() const override { return true; }
    bool fix(const LintIssue& issue) override;

private:
    struct ConstCandidate {
        std::string name;
        std::string type;
        std::string file;
        int line;
        std::string init_expr;
        bool can_be_constexpr;
    };

    std::vector<ConstCandidate> find_candidates(const std::string& filepath);
};

}  // namespace lint
}  // namespace codelint