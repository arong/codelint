#include "lint/lint_checker.h"
#include "lint/checkers/init_checker.h"
#include "lint/checkers/const_checker.h"
#include "lint/checkers/global_checker.h"
#include "lint/checkers/singleton_checker.h"

namespace codelint {
namespace lint {

std::unique_ptr<LintChecker> CheckerFactory::create(const std::string& name) {
    if (name == "init") {
        return std::make_unique<InitChecker>();
    } else if (name == "const") {
        return std::make_unique<ConstChecker>();
    } else if (name == "global") {
        return std::make_unique<GlobalChecker>();
    } else if (name == "singleton") {
        return std::make_unique<SingletonChecker>();
    }
    return nullptr;
}

std::vector<std::unique_ptr<LintChecker>> CheckerFactory::create_all() {
    std::vector<std::unique_ptr<LintChecker>> checkers;
    checkers.push_back(std::make_unique<InitChecker>());
    checkers.push_back(std::make_unique<ConstChecker>());
    checkers.push_back(std::make_unique<GlobalChecker>());
    checkers.push_back(std::make_unique<SingletonChecker>());
    return checkers;
}

std::vector<std::string> CheckerFactory::available_checkers() {
    return {"init", "const", "global", "singleton"};
}

}  // namespace lint
}  // namespace codelint
