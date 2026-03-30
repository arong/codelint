#pragma once

#include "commands.h"

// Forward declaration
namespace codelint {
namespace lint {
class GlobalChecker;
}
}  // namespace codelint

/**
 * Find global variables in the specified path.
 *
 * @param opts Global options (output_json, scope, etc.)
 * @param global_opts Options specific to find_global command (path)
 * @return Exit code (0 for success, non-zero for error)
 */
int find_global(const GlobalOptions& opts, const FindGlobalOptions& global_opts);
