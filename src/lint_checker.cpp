#include "lint/lint_checker.h"
#include "lint/checkers/const_checker.h"
#include "lint/checkers/global_checker.h"
#include "lint/checkers/init_checker.h"
#include "lint/checkers/singleton_checker.h"

namespace codelint {
namespace lint {

std::unique_ptr<LintChecker> CheckerFactory::create(const std::string& name,
                                                    const std::optional<GitScope>& scope) {
  if (name == "init") {
    return std::make_unique<InitChecker>(scope);
  } else if (name == "const") {
    return std::make_unique<ConstChecker>(scope);
  } else if (name == "global") {
    return std::make_unique<GlobalChecker>(scope);
  } else if (name == "singleton") {
    return std::make_unique<SingletonChecker>(scope);
  }
  return nullptr;
}

std::vector<std::unique_ptr<LintChecker>>
CheckerFactory::create_all(const std::optional<GitScope>& scope) {
  std::vector<std::unique_ptr<LintChecker>> checkers;
  checkers.push_back(std::make_unique<InitChecker>(scope));
  checkers.push_back(std::make_unique<ConstChecker>(scope));
  checkers.push_back(std::make_unique<GlobalChecker>(scope));
  checkers.push_back(std::make_unique<SingletonChecker>(scope));
  return checkers;
}

std::vector<std::string> CheckerFactory::available_checkers() {
  return {"init", "const", "global", "singleton"};
}

} // namespace lint
} // namespace codelint
