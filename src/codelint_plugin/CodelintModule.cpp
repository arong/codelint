#include "codelint/Compatibility.h"
#include "codelint/checks/FunctionSizeCheck.h"
#include "codelint/checks/GlobalConstStringCheck.h"
#include "codelint/checks/InitCheck.h"
#include "codelint/checks/LintCodeCheck.h"
#include "codelint/checks/LocalVarNamingCheck.h"
#include "codelint/checks/SignedToUnsignedReturnCheck.h"
#ifndef CODELINT_DISABLE_SINGLETON_CHECK
#include "codelint/checks/SingletonCheck.h"
#endif
#include "codelint/checks/LogTagMismatchCheck.h"
#include "codelint/checks/StrictBoolConditionCheck.h"
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

namespace clang::tidy::codelint {

class CodelintModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override {
    CheckFactories.registerCheck<InitCheck>("codelint-init");
    CheckFactories.registerCheck<LintCodeCheck>("codelint-lint-code");
    CheckFactories.registerCheck<LocalVarNamingCheck>("codelint-local-var-naming");
    CheckFactories.registerCheck<FunctionSizeCheck>("codelint-function-size");
    CheckFactories.registerCheck<GlobalConstStringCheck>("codelint-global-const-string");
#ifndef CODELINT_DISABLE_SINGLETON_CHECK
    CheckFactories.registerCheck<SingletonCheck>("codelint-singleton");
#endif
    CheckFactories.registerCheck<StrictBoolConditionCheck>("codelint-strict-bool-condition");
    CheckFactories.registerCheck<SignedToUnsignedReturnCheck>("codelint-signed-to-unsigned-return");
    CheckFactories.registerCheck<LogTagMismatchCheck>("codelint-log-tag-mismatch");
  }
};

} // namespace clang::tidy::codelint

// Register the module
static clang::tidy::ClangTidyModuleRegistry::Add<clang::tidy::codelint::CodelintModule>
    X("codelint-module",
      "Adds codelint checks: init, lint-code, local-var-naming, function-size, "
      "global-const-string, singleton"); // NOLINT
