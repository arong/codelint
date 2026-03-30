#pragma once

#include "commands.h"

namespace codelint {
namespace lint {
class InitChecker;
class ConstChecker;
}  // namespace lint
}  // namespace codelint

int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts);
