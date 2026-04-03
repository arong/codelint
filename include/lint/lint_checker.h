#pragma once

#include "git_scope.h"
#include "lint_types.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace codelint {
namespace lint {

class LintChecker {
public:
  virtual ~LintChecker() = default;

  virtual LintResult check(const std::string& filepath) = 0;

  virtual std::string name() const = 0;
  virtual std::string description() const = 0;
  virtual std::vector<CheckType> provides() const = 0;

  virtual bool can_fix() const {
    return false;
  }
  virtual bool apply_fixes(const std::string& filepath, const std::vector<LintIssue>& issues,
                           std::string& modified_content) {
    return false;
  }
};

class CheckerFactory {
public:
  static std::unique_ptr<LintChecker> create(const std::string& name,
                                             const std::optional<GitScope>& scope = std::nullopt);
  static std::vector<std::unique_ptr<LintChecker>>
  create_all(const std::optional<GitScope>& scope = std::nullopt);
  static std::vector<std::string> available_checkers();
};

} // namespace lint
} // namespace codelint
