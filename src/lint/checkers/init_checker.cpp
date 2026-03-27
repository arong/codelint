#include "lint/checkers/init_checker.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace codelint {
namespace lint {

LintResult InitChecker::check(const std::string& filepath) {
    LintResult result;
    
    CXIndex index = clang_createIndex(0, 0);
    if (!index) {
        return result;
    }
    
    CXTranslationUnit tu = parse_file(index, filepath);
    if (!tu) {
        clang_disposeIndex(index);
        return result;
    }
    
    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    visit_translation_unit(cursor, result);
    
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    
    return result;
}

void InitChecker::visit_translation_unit(CXCursor cursor, LintResult& result) {
    CXCursorKind kind = clang_getCursorKind(cursor);
    
    if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
        visit_function_body(cursor, result);
    } else {
        clang_visitChildren(
            cursor,
            [](CXCursor c, CXCursor parent, CXClientData data) {
                auto* res = static_cast<LintResult*>(data);
                auto* self = const_cast<InitChecker*>(
                    static_cast<const InitChecker*>(nullptr));
                reinterpret_cast<InitChecker*>(data)->visit_translation_unit(c, *res);
                return CXChildVisit_Continue;
            },
            &result);
    }
}

void InitChecker::visit_function_body(CXCursor cursor, LintResult& result) {
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
            auto* res = static_cast<LintResult*>(data);
            if (clang_getCursorKind(c) == CXCursor_VarDecl) {
                auto* self = reinterpret_cast<InitChecker*>(
                    reinterpret_cast<uintptr_t*>(data)[1]);
                self->check_var_decl(c, *res);
            }
            return CXChildVisit_Recurse;
        },
        &result);
}

void InitChecker::check_var_decl(CXCursor cursor, LintResult& result) {
    std::string var_name = ClangUtils::get_cursor_spelling(cursor);
    std::string type_str = ClangUtils::get_type_spelling(cursor);
    std::string file = ClangUtils::get_file_path(cursor);
    
    if (var_name.empty() || type_str.empty() || ClangUtils::is_system_header(file)) {
        return;
    }
    
    int line = ClangUtils::get_line_number(cursor);
    int column = ClangUtils::get_column_number(cursor);
    
    CXCursor init_cursor = clang_Cursor_getVarDeclInitializer(cursor);
    
    if (clang_Cursor_isNull(init_cursor)) {
        check_uninitialized(cursor, var_name, type_str, result);
    } else {
        check_equals_init(cursor, init_cursor, var_name, type_str, result);
    }
}

void InitChecker::check_uninitialized(CXCursor cursor, const std::string& var_name,
                                      const std::string& type_str, LintResult& result) {
    LintIssue issue;
    issue.type = CheckType::INIT_UNINITIALIZED;
    issue.severity = Severity::WARNING;
    issue.checker_name = "init";
    issue.name = var_name;
    issue.type_str = type_str;
    issue.file = ClangUtils::get_file_path(cursor);
    issue.line = ClangUtils::get_line_number(cursor);
    issue.column = ClangUtils::get_column_number(cursor);
    issue.description = "Variable is not explicitly initialized";
    issue.suggestion = type_str + " " + var_name + "{};";
    issue.fixable = true;
    result.add_issue(issue);
}

void InitChecker::check_equals_init(CXCursor cursor, CXCursor init_cursor,
                                    const std::string& var_name,
                                    const std::string& type_str, LintResult& result) {
    CXCursorKind init_kind = clang_getCursorKind(init_cursor);
    bool is_brace_init = (init_kind == CXCursor_InitListExpr);
    
    if (!is_brace_init) {
        std::string file = ClangUtils::get_file_path(cursor);
        int line = ClangUtils::get_line_number(cursor);
        int column = ClangUtils::get_column_number(cursor);
        std::string init_value = ClangUtils::get_init_value(init_cursor);
        bool is_unsigned = is_unsigned_type(type_str);
        
        LintIssue issue;
        issue.type = CheckType::INIT_EQUALS_SYNTAX;
        issue.severity = Severity::INFO;
        issue.checker_name = "init";
        issue.name = var_name;
        issue.type_str = type_str;
        issue.file = file;
        issue.line = line;
        issue.column = column;
        issue.description = "Variable initialized with '=' should use '{}' syntax";
        issue.fixable = true;
        
        if (is_unsigned && has_digit_value(init_value) && !has_unsigned_suffix(init_value)) {
            issue.suggestion = type_str + " " + var_name + "{" + init_value + "U};";
        } else {
            issue.suggestion = type_str + " " + var_name + "{" + init_value + "};";
        }
        result.add_issue(issue);
    }
    
    bool is_unsigned = is_unsigned_type(type_str);
    if (is_unsigned && !is_brace_init) {
        check_unsigned_suffix(cursor, var_name, type_str, result);
    }
}

void InitChecker::check_unsigned_suffix(CXCursor cursor, const std::string& var_name,
                                        const std::string& type_str, LintResult& result) {
    CXCursor init_cursor = clang_Cursor_getVarDeclInitializer(cursor);
    if (clang_Cursor_isNull(init_cursor)) return;
    
    std::string init_value = ClangUtils::get_init_value(init_cursor);
    
    if (has_digit_value(init_value) && !has_unsigned_suffix(init_value)) {
        LintIssue issue;
        issue.type = CheckType::INIT_UNSIGNED_SUFFIX;
        issue.severity = Severity::HINT;
        issue.checker_name = "init";
        issue.name = var_name;
        issue.type_str = type_str;
        issue.file = ClangUtils::get_file_path(cursor);
        issue.line = ClangUtils::get_line_number(cursor);
        issue.column = ClangUtils::get_column_number(cursor);
        issue.description = "Unsigned integer should have 'U' or 'UL' suffix";
        issue.suggestion = "";
        issue.fixable = false;
        result.add_issue(issue);
    }
}

bool InitChecker::is_unsigned_type(const std::string& type_str) const {
    return type_str.find("unsigned") != std::string::npos;
}

bool InitChecker::has_digit_value(const std::string& value) const {
    return std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

bool InitChecker::has_unsigned_suffix(const std::string& value) const {
    return value.find('U') != std::string::npos || value.find('u') != std::string::npos;
}

bool InitChecker::apply_fixes(const std::string& filepath,
                              const std::vector<LintIssue>& issues,
                              std::string& modified_content) {
    std::ifstream input_file(filepath);
    if (!input_file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    modified_content = buffer.str();
    input_file.close();
    
    std::vector<std::string> lines;
    std::stringstream ss(modified_content);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    for (const auto& issue : issues) {
        if (issue.checker_name != "init") continue;
        if (issue.line <= 0 || issue.line > (int)lines.size()) continue;
        
        int idx = issue.line - 1;
        
        if (issue.type == CheckType::INIT_UNINITIALIZED) {
            size_t pos = lines[idx].find(issue.name);
            if (pos != std::string::npos) {
                size_t name_end = pos + issue.name.length();
                while (name_end < lines[idx].length() && 
                       (lines[idx][name_end] == ' ' || lines[idx][name_end] == '\t')) {
                    name_end++;
                }
                if (name_end < lines[idx].length() && lines[idx][name_end] == ';') {
                    lines[idx].insert(name_end, "{}");
                }
            }
        } else if (issue.type == CheckType::INIT_EQUALS_SYNTAX) {
            size_t pos = lines[idx].find("=");
            if (pos != std::string::npos) {
                size_t comment_pos = lines[idx].find("//");
                if (comment_pos == std::string::npos || comment_pos > pos) {
                    size_t end_pos = lines[idx].find(";", pos);
                    if (end_pos != std::string::npos) {
                        size_t replace_start = pos;
                        while (replace_start > 0 && 
                               (lines[idx][replace_start - 1] == ' ' || lines[idx][replace_start - 1] == '\t')) {
                            replace_start--;
                        }
                        
                        std::string init_part = lines[idx].substr(pos + 1, end_pos - pos - 1);
                        size_t val_start = init_part.find_first_not_of(" \t");
                        if (val_start != std::string::npos) {
                            init_part = init_part.substr(val_start);
                        }
                        size_t val_end = init_part.find_last_not_of(" \t");
                        if (val_end != std::string::npos) {
                            init_part = init_part.substr(0, val_end + 1);
                        }
                        
                        std::string replacement = "{" + init_part + "};";
                        lines[idx].replace(replace_start, end_pos - replace_start + 1, replacement);
                    }
                }
            }
        }
    }
    
    std::stringstream output;
    for (const auto& l : lines) {
        output << l << "\n";
    }
    modified_content = output.str();
    return true;
}

}  // namespace lint
}  // namespace codelint