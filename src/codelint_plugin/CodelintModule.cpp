#include "codelint/checks/GlobalCheck.h"
#include "codelint/checks/InitCheck.h"
#include "codelint/checks/SingletonCheck.h"
#include "codelint/checks/StrictBoolConditionCheck.h"
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

namespace clang::tidy::codelint {

class CodelintModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
    CheckFactories.registerCheck<InitCheck>("codelint-init");
    CheckFactories.registerCheck<GlobalCheck>("codelint-global");
    CheckFactories.registerCheck<SingletonCheck>("codelint-singleton");
    CheckFactories.registerCheck<StrictBoolConditionCheck>("codelint-strict-bool-condition");
  }
};

} // namespace clang::tidy::codelint

// Register the module
static clang::tidy::ClangTidyModuleRegistry::Add<clang::tidy::codelint::CodelintModule>
    X("codelint-module", "Adds codelint checks: init, global, singleton"); // NOLINT
