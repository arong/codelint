#include "clang-tidy/ClangTidyModule.h"
#include "clang-tidy/ClangTidyModuleRegistry.h"
#include "codelint/checks/GlobalCheck.h"
#include "codelint/checks/InitCheck.h"
#include "codelint/checks/SingletonCheck.h"
#include "codelint/checks/StrictBoolConditionCheck.h"

namespace clang::tidy {
namespace codelint {

class CodelintModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
    CheckFactories.registerCheck<InitCheck>("codelint-init");
    CheckFactories.registerCheck<GlobalCheck>("codelint-global");
    CheckFactories.registerCheck<SingletonCheck>("codelint-singleton");
    CheckFactories.registerCheck<StrictBoolConditionCheck>("codelint-strict-bool-condition");
  }
};

} // namespace codelint
} // namespace clang::tidy

// Register the module
static clang::tidy::ClangTidyModuleRegistry::Add<clang::tidy::codelint::CodelintModule>
    X("codelint-module", "Adds codelint checks: init, global, singleton"); // NOLINT
