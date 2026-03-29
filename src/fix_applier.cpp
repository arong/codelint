#include "lint/fix_applier.h"
#include <clang/Tooling/Core/Replacement.h>
#include <sstream>
#include <algorithm>

namespace codelint {
namespace lint {

bool FixApplier::applyFixes(const std::vector<LintIssue>& issues,
                            const std::string& original_content,
                            std::string& modified_content,
                            const std::string& file_path) {
    applied_fix_count_ = 0;

    std::vector<LintIssue> fixable_issues;
    for (const auto& issue : issues) {
        if (issue.fixable &&
            (issue.type == CheckType::INIT_UNINITIALIZED ||
             issue.type == CheckType::INIT_EQUALS_SYNTAX)) {
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
        }

        if (applied && !replacement_text.empty()) {
            llvm::Error err = replacements.add(clang::tooling::Replacement(
                file_path, offset, length, replacement_text));
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
                                       std::string& replacement_text,
                                       size_t& offset,
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

    offset = offset + insert_pos;
    length = 0;
    replacement_text = "{}";

    return true;
}

bool FixApplier::applyEqualsSyntaxFix(const LintIssue& issue,
                                      const std::vector<std::string>& lines,
                                      std::string& replacement_text,
                                      size_t& offset,
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
    } else {
        replacement_text = "{" + init_part + "};";
    }

    return true;
}

size_t FixApplier::findInsertionPoint(const std::string& line,
                                      const std::string& var_name,
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

}  // namespace lint
}  // namespace codelint
