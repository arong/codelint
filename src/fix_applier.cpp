#include "lint/fix_applier.h"
#include <algorithm>
#include <clang/Tooling/Core/Replacement.h>
#include <sstream>

namespace codelint {
namespace lint {

bool FixApplier::applyFixes(const std::vector<LintIssue>& issues,
                            const std::string& original_content, std::string& modified_content,
                            const std::string& file_path) {
  applied_fix_count_ = 0;

  std::vector<LintIssue> fixable_issues;
  for (const auto& issue : issues) {
    if (issue.fixable && (issue.type == CheckType::INIT_UNINITIALIZED ||
                          issue.type == CheckType::INIT_EQUALS_SYNTAX ||
                          issue.type == CheckType::INIT_UNSIGNED_SUFFIX ||
                          issue.type == CheckType::CONST_SUGGESTION ||
                          issue.type == CheckType::CAN_BE_CONST ||
                          issue.type == CheckType::CAN_BE_CONSTEXPR)) {
      fixable_issues.push_back(issue);
    }
  }

  if (fixable_issues.empty()) {
    modified_content = original_content;
    return false;
  }

  std::vector<std::string> lines;
  std::istringstream stream(original_content);
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }

  std::vector<size_t> line_offsets;
  line_offsets.push_back(0);
  size_t current_offset = 0;
  for (const auto& l : lines) {
    current_offset += l.length() + 1;
    line_offsets.push_back(current_offset);
  }

  // Process issues from end to start to prevent offset shifts from affecting earlier replacements
  std::sort(fixable_issues.begin(), fixable_issues.end(),
            [&line_offsets](const LintIssue& a, const LintIssue& b) {
              size_t offset_a = line_offsets[a.line - 1] + a.column - 1;
              size_t offset_b = line_offsets[b.line - 1] + b.column - 1;
              return offset_a > offset_b;
            });

  clang::tooling::Replacements replacements;

  for (const auto& issue : fixable_issues) {
    int line_idx = issue.line - 1;
    if (line_idx < 0 || line_idx >= static_cast<int>(lines.size())) {
      continue;
    }

    size_t offset = line_offsets[line_idx];
    size_t length = 0;
    std::string replacement_text;

    bool applied = false;

    if (issue.type == CheckType::INIT_UNINITIALIZED) {
      applied = applyUninitializedFix(issue, lines, replacement_text, offset, length);
    } else if (issue.type == CheckType::INIT_EQUALS_SYNTAX) {
      applied = applyEqualsSyntaxFix(issue, lines, replacement_text, offset, length);
    } else if (issue.type == CheckType::INIT_UNSIGNED_SUFFIX) {
      applied = applyUnsignedSuffixFix(issue, lines, replacement_text, offset, length);
    } else if (issue.type == CheckType::CONST_SUGGESTION ||
               issue.type == CheckType::CAN_BE_CONST ||
               issue.type == CheckType::CAN_BE_CONSTEXPR) {
      applied = applyConstSuggestionFix(issue, lines, replacement_text, offset, length);
    }

    if (applied && !replacement_text.empty()) {
      llvm::Error err = replacements.add(
          clang::tooling::Replacement(file_path, offset, length, replacement_text));
      if (!err) {
        applied_fix_count_++;
      } else {
        llvm::consumeError(std::move(err));
      }
    }
  }

  if (replacements.empty()) {
    modified_content = original_content;
    return false;
  }

  auto result = clang::tooling::applyAllReplacements(original_content, replacements);
  if (!result) {
    modified_content = original_content;
    return false;
  }

  modified_content = *result;
  return applied_fix_count_ > 0;
}

bool FixApplier::applyUninitializedFix(const LintIssue& issue,
                                       const std::vector<std::string>& lines,
                                       std::string& replacement_text, size_t& offset,
                                       size_t& length) {
  int line_idx = issue.line - 1;
  if (line_idx < 0 || line_idx >= static_cast<int>(lines.size())) {
    return false;
  }

  const std::string& line = lines[line_idx];

  size_t var_pos = line.find(issue.name);
  if (var_pos == std::string::npos) {
    return false;
  }

  size_t insert_pos = findInsertionPoint(line, issue.name, var_pos);
  if (insert_pos == std::string::npos) {
    return false;
  }

  if (insert_pos < line.length() && line[insert_pos] == '{') {
    return false;
  }

  offset = offset + insert_pos;
  length = 0;
  replacement_text = "{}";

  return true;
}

bool FixApplier::applyEqualsSyntaxFix(const LintIssue& issue, const std::vector<std::string>& lines,
                                      std::string& replacement_text, size_t& offset,
                                      size_t& length) {
  int line_idx = issue.line - 1;
  if (line_idx < 0 || line_idx >= static_cast<int>(lines.size())) {
    return false;
  }

  const std::string& line = lines[line_idx];

  size_t var_pos = line.find(issue.name);
  if (var_pos == std::string::npos) {
    return false;
  }

  size_t equals_pos = line.find('=', var_pos + issue.name.length());
  if (equals_pos == std::string::npos) {
    return false;
  }

  size_t comment_pos = line.find("//");
  if (comment_pos != std::string::npos && comment_pos < equals_pos) {
    return false;
  }

  size_t semicolon_pos = line.find(';', equals_pos);
  if (semicolon_pos == std::string::npos) {
    return false;
  }

  size_t replace_start = equals_pos;
  while (replace_start > 0 && (line[replace_start - 1] == ' ' || line[replace_start - 1] == '\t')) {
    replace_start--;
  }

  std::string init_part = line.substr(equals_pos + 1, semicolon_pos - equals_pos - 1);

  size_t val_start = init_part.find_first_not_of(" \t");
  if (val_start != std::string::npos) {
    init_part = init_part.substr(val_start);
  }

  size_t val_end = init_part.find_last_not_of(" \t");
  if (val_end != std::string::npos) {
    init_part = init_part.substr(0, val_end + 1);
  }

  offset = offset + replace_start;
  length = semicolon_pos - replace_start + 1;

  if (init_part.size() >= 2 && init_part.front() == '{' && init_part.back() == '}') {
    replacement_text = init_part + ";";
  } else if (!issue.suggestion.empty()) {
    size_t indent_end = line.find_first_not_of(" \t");
    std::string indent = (indent_end != std::string::npos) ? line.substr(0, indent_end) : "";
    replacement_text = issue.suggestion;
    size_t brace_pos = replacement_text.find('{');
    if (brace_pos != std::string::npos) {
      size_t close_brace_pos = replacement_text.find('}', brace_pos);
      if (close_brace_pos != std::string::npos) {
        replacement_text = replacement_text.substr(brace_pos, close_brace_pos - brace_pos + 2);
      }
    }
  } else {
    replacement_text = "{" + init_part + "};";
  }

  return true;
}

bool FixApplier::applyUnsignedSuffixFix(const LintIssue& issue,
                                        const std::vector<std::string>& lines,
                                        std::string& replacement_text, size_t& offset,
                                        size_t& length) {
  int line_idx = issue.line - 1;
  if (line_idx < 0 || line_idx >= static_cast<int>(lines.size())) {
    return false;
  }

  const std::string& line = lines[line_idx];

  size_t var_pos = line.find(issue.name);
  if (var_pos == std::string::npos) {
    return false;
  }

  size_t open_brace_pos = line.find('{', var_pos + issue.name.length());
  if (open_brace_pos == std::string::npos) {
    return false;
  }

  size_t close_brace_pos = line.find('}', open_brace_pos);
  if (close_brace_pos == std::string::npos) {
    return false;
  }

  std::string value_part = line.substr(open_brace_pos + 1, close_brace_pos - open_brace_pos - 1);

  size_t val_start = value_part.find_first_not_of(" \t");
  if (val_start != std::string::npos) {
    value_part = value_part.substr(val_start);
  }

  size_t val_end = value_part.find_last_not_of(" \t");
  if (val_end != std::string::npos) {
    value_part = value_part.substr(0, val_end + 1);
  }

  if (value_part.find('U') != std::string::npos || value_part.find('u') != std::string::npos) {
    return false;
  }

  offset = offset + open_brace_pos + 1;
  length = close_brace_pos - open_brace_pos - 1;
  replacement_text = value_part + "U";

  return true;
}

size_t FixApplier::findInsertionPoint(const std::string& line, const std::string& var_name,
                                      size_t start_pos) {
  size_t var_end = start_pos + var_name.length();

  while (var_end < line.length() && (line[var_end] == ' ' || line[var_end] == '\t')) {
    var_end++;
  }

  var_end = skipArrayDimensions(line, var_end);

  if (var_end < line.length() && (line[var_end] == ';' || line[var_end] == ',')) {
    return var_end;
  }

  if (var_end < line.length() && line[var_end] == '(') {
    return std::string::npos;
  }

  return var_end;
}

size_t FixApplier::skipArrayDimensions(const std::string& line, size_t start_pos) {
  size_t pos = start_pos;

  while (pos < line.length() && line[pos] == '[') {
    int bracket_count = 1;
    size_t current = pos + 1;

    while (current < line.length() && bracket_count > 0) {
      if (line[current] == '[') {
        bracket_count++;
      } else if (line[current] == ']') {
        bracket_count--;
      }
      current++;
    }

    pos = current;

    while (pos < line.length() && (line[pos] == ' ' || line[pos] == '\t')) {
      pos++;
    }
  }

  return pos;
}

bool FixApplier::isArrayDimensionStart(const std::string& line, size_t pos) {
  return pos < line.length() && line[pos] == '[';
}

bool FixApplier::applyConstSuggestionFix(const LintIssue& issue,
                                         const std::vector<std::string>& lines,
                                         std::string& replacement_text, size_t& offset,
                                         size_t& length) {
  int line_idx = issue.line - 1;
  if (line_idx < 0 || line_idx >= static_cast<int>(lines.size())) {
    return false;
  }

  const std::string& line = lines[line_idx];

  // Find the variable name
  size_t var_pos = line.find(issue.name);
  if (var_pos == std::string::npos) {
    return false;
  }

  // Find the type position (search backwards from variable name)
  // First try the full type_str
  size_t type_pos = line.rfind(issue.type_str, var_pos);
  
  // If not found, the type might have different spacing (e.g., "int &" vs "int&")
  // Try finding just the base type without spaces and reference/pointer markers
   if (type_pos == std::string::npos) {
    std::string base_type = issue.type_str;
    // Remove spaces
    base_type.erase(std::remove(base_type.begin(), base_type.end(), ' '), base_type.end());
    // Remove array dimensions [...]
    size_t bracket_pos = base_type.find('[');
    if (bracket_pos != std::string::npos) {
      base_type = base_type.substr(0, bracket_pos);
    }
    // Remove reference and pointer markers
    size_t ref_pos = base_type.find_first_of("&*");
    if (ref_pos != std::string::npos) {
      base_type = base_type.substr(0, ref_pos);
    }
    // Search for base type in line
    type_pos = line.rfind(base_type, var_pos);
  }
  
  if (type_pos == std::string::npos) {
    return false;
  }

  if (issue.type == CheckType::CAN_BE_CONSTEXPR) {
    // Check if type_str starts with "const" - this is a const -> constexpr upgrade
    // We need to replace "const " with "constexpr " in the source
    if (issue.type_str.find("const") == 0) {
      // Look for "const " in the line at or before type_pos
      size_t const_space_pos = line.rfind("const ", type_pos + 6);
      if (const_space_pos != std::string::npos && const_space_pos <= type_pos) {
        // Replace "const " with "constexpr "
        offset = offset + const_space_pos;
        length = 6; // "const " length
        replacement_text = "constexpr ";
        return true;
      }
      // Also try "const" without trailing space (followed directly by type)
      size_t const_pos = line.rfind("const", type_pos + 5);
      if (const_pos != std::string::npos && const_pos == type_pos) {
        offset = offset + const_pos;
        length = 5; // "const" length
        replacement_text = "constexpr ";
        return true;
      }
    }
    // No existing const, insert "constexpr " before type
    offset = offset + type_pos;
    length = 0;
    replacement_text = "constexpr ";
    return true;
  } else {
    // Regular const suggestion - insert "const " before type
    offset = offset + type_pos;
    length = 0;
    replacement_text = "const ";
    return true;
  }
}

} // namespace lint
} // namespace codelint
