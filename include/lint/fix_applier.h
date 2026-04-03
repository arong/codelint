#ifndef CNDY_FIX_APPLIER_H
#define CNDY_FIX_APPLIER_H

#include "lint/lint_types.h"
#include <map>
#include <string>
#include <vector>

namespace codelint {
namespace lint {

/**
 * FixApplier applies automatic fixes to source code using clang::tooling::Replacements.
 *
 * Handles fixable LintIssues by:
 * - INIT_UNINITIALIZED: Insert "{}" after variable name
 * - INIT_EQUALS_SYNTAX: Replace "= value" with "{value}"
 *
 * Correctly handles:
 * - Array dimensions (inserts {} after [N] not before)
 * - Multiple variables on same line
 * - Overlapping replacements (Replacements class handles this)
 */
class FixApplier {
public:
  FixApplier() = default;
  ~FixApplier() = default;

  FixApplier(const FixApplier&) = delete;
  FixApplier& operator=(const FixApplier&) = delete;

  FixApplier(FixApplier&&) = default;
  FixApplier& operator=(FixApplier&&) = default;

  /**
   * Apply fixes to a source file.
   *
   * @param issues List of lint issues to fix (only fixable ones are processed)
   * @param original_content The original source code content
   * @param modified_content Output parameter for the fixed content
   * @param file_path Path to the file (for replacement tracking)
   * @return true if any fixes were applied, false otherwise
   */
  bool applyFixes(const std::vector<LintIssue>& issues, const std::string& original_content,
                  std::string& modified_content, const std::string& file_path);

  /**
   * Get the count of fixes applied in the last call to applyFixes.
   */
  int getAppliedFixCount() const {
    return applied_fix_count_;
  }

private:
  int applied_fix_count_ = 0;

  /**
   * Apply fix for INIT_UNINITIALIZED issue.
   * Inserts "{}" after the variable name, accounting for array dimensions.
   */
  bool applyUninitializedFix(const LintIssue& issue, const std::vector<std::string>& lines,
                             std::string& replacement_text, size_t& offset, size_t& length);

  /**
   * Apply fix for INIT_EQUALS_SYNTAX issue.
   * Replaces "= value" with "{value}".
   */
  bool applyEqualsSyntaxFix(const LintIssue& issue, const std::vector<std::string>& lines,
                            std::string& replacement_text, size_t& offset, size_t& length);

  bool applyUnsignedSuffixFix(const LintIssue& issue, const std::vector<std::string>& lines,
                              std::string& replacement_text, size_t& offset, size_t& length);

  /**
   * Find the position to insert "{}" for a variable, handling arrays.
   * Returns the column position (0-indexed) where {} should be inserted.
   */
  size_t findInsertionPoint(const std::string& line, const std::string& var_name, size_t start_pos);

  /**
   * Check if a position in line is the start of an array dimension.
   */
  bool isArrayDimensionStart(const std::string& line, size_t pos);

  /**
   * Find the matching closing bracket for an array dimension.
   * Returns the position after the closing bracket.
   */
  size_t skipArrayDimensions(const std::string& line, size_t start_pos);
};

} // namespace lint
} // namespace codelint

#endif // CNDY_FIX_APPLIER_H
