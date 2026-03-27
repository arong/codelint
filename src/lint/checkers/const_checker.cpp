#include "lint/checkers/const_checker.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

namespace codelint {
namespace lint {

LintResult ConstChecker::check(const std::string& filepath) {
    LintResult result;
    variables_.clear();
    modified_vars_.clear();
    
    CXIndex index = clang_createIndex(0, 0);
    if (!index) return result;
    
    CXTranslationUnit tu = parse_file(index, filepath);
    if (!tu) { clang_disposeIndex(index); return result; }
    
    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    visit_cursor(cursor, filepath, result);
    
    analyze_and_report(filepath, result);
    
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    
    return result;
}

struct VisitorData {
    ConstChecker* checker;
    const std::string* filepath;
    LintResult* result;
};

static CXChildVisitResult visitor_callback(CXCursor cursor, CXCursor parent, CXClientData client_data) {
    VisitorData* data = static_cast<VisitorData*>(client_data);
    CXCursorKind kind = clang_getCursorKind(cursor);
    
    if (kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl) {
        data->checker->collect_variables(cursor, *data->filepath, *data->result);
    }
    
    if (kind == CXCursor_BinaryOperator || kind == CXCursor_UnaryOperator || kind == CXCursor_CallExpr) {
        data->checker->track_modifications(cursor);
    }
    
    return CXChildVisit_Recurse;
}

void ConstChecker::visit_cursor(CXCursor cursor, const std::string& filepath, LintResult& result) {
    VisitorData data = {this, &filepath, &result};
    clang_visitChildren(cursor, visitor_callback, &data);
}

void ConstChecker::collect_variables(CXCursor cursor, const std::string&, LintResult&) {
    std::string name = ClangUtils::get_cursor_spelling(cursor);
    std::string type = ClangUtils::get_type_spelling(cursor);
    std::string file = ClangUtils::get_file_path(cursor);
    
    if (name.empty() || ClangUtils::is_system_header(file)) return;
    
    VarInfo info;
    info.name = name; info.type = type; info.file = file;
    info.line = ClangUtils::get_line_number(cursor);
    info.column = ClangUtils::get_column_number(cursor);
    
    CXType cx_type = clang_getCursorType(cursor);
    info.is_const = clang_isConstQualifiedType(cx_type);
    info.is_reference = (cx_type.kind == CXType_LValueReference || cx_type.kind == CXType_RValueReference);
    info.is_pointer = (cx_type.kind == CXType_Pointer);
    info.is_member = (clang_getCursorKind(cursor) == CXCursor_FieldDecl);
    
    CXCursor parent = clang_getCursorSemanticParent(cursor);
    info.is_global = (clang_getCursorKind(parent) == CXCursor_TranslationUnit);
    
    std::string key = file + ":" + std::to_string(info.line) + ":" + name;
    variables_[key] = info;
}

void ConstChecker::track_modifications(CXCursor cursor) {
    std::string name = ClangUtils::get_cursor_spelling(cursor);
    if (!name.empty()) modified_vars_.insert(name);
}

void ConstChecker::analyze_and_report(const std::string&, LintResult& result) {
    for (auto& [key, info] : variables_) {
        if (info.is_const || info.is_constexpr) continue;
        if (info.is_reference || info.is_pointer) continue;
        if (info.is_member || info.is_global) continue;
        if (modified_vars_.count(info.name) > 0) continue;
        if (!is_builtin_type(info.type)) continue;
        
        LintIssue issue;
        issue.type = CheckType::CONST_SUGGESTION;
        issue.severity = Severity::HINT;
        issue.checker_name = "const";
        issue.name = info.name;
        issue.type_str = info.type;
        issue.file = info.file;
        issue.line = info.line;
        issue.column = info.column;
        issue.description = "Variable is never modified, consider making it const";
        issue.suggestion = make_const_suggestion(info);
        issue.fixable = true;
        result.add_issue(issue);
    }
}

bool ConstChecker::is_builtin_type(const std::string& type) const {
    static const std::vector<std::string> builtin_types = {
        "int", "unsigned int", "long", "unsigned long", "long long",
        "unsigned long long", "short", "unsigned short", "char",
        "unsigned char", "signed char", "float", "double", "long double",
        "bool", "wchar_t", "char16_t", "char32_t", "size_t", "int8_t",
        "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t"
    };
    
    std::string clean_type = type;
    size_t pos = clean_type.find("const ");
    while (pos != std::string::npos) { clean_type.erase(pos, 6); pos = clean_type.find("const "); }
    while (!clean_type.empty() && (clean_type.back() == ' ' || clean_type.back() == '&')) clean_type.pop_back();
    
    return std::find(builtin_types.begin(), builtin_types.end(), clean_type) != builtin_types.end();
}

bool ConstChecker::can_be_constexpr(const VarInfo& info) const {
    return is_builtin_type(info.type) && !modified_vars_.count(info.name);
}

std::string ConstChecker::make_const_suggestion(const VarInfo& info) const {
    return "const " + info.type + " " + info.name;
}

bool ConstChecker::apply_fixes(const std::string& filepath, const std::vector<LintIssue>& issues, std::string& modified_content) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    std::stringstream buffer; buffer << file.rdbuf(); modified_content = buffer.str();
    file.close();
    
    std::vector<std::string> lines;
    std::stringstream ss(modified_content); std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    
    for (const auto& issue : issues) {
        if (issue.checker_name != "const" || issue.line <= 0 || issue.line > (int)lines.size()) continue;
        int idx = issue.line - 1;
        size_t pos = lines[idx].find(issue.name);
        if (pos != std::string::npos) {
            size_t type_pos = lines[idx].rfind(issue.type_str, pos);
            if (type_pos != std::string::npos) lines[idx].insert(type_pos, "const ");
        }
    }
    
    std::stringstream output;
    for (const auto& l : lines) output << l << "\n";
    modified_content = output.str();
    return true;
}

}  // namespace lint
}  // namespace codelint
