#include "lint/checkers/singleton_checker.h"

namespace codelint {
namespace lint {

LintResult SingletonChecker::check(const std::string& filepath) {
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
    visit_cursor(cursor, result);
    
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    
    return result;
}

void SingletonChecker::visit_cursor(CXCursor cursor, LintResult& result) {
    CXCursorKind kind = clang_getCursorKind(cursor);
    
    if (kind == CXCursor_CXXMethod) {
        std::string method_name = ClangUtils::get_cursor_spelling(cursor);
        
        if (method_name == "instance" || method_name == "getInstance") {
            CXType result_type = clang_getCursorResultType(cursor);
            
            if (result_type.kind == CXType_LValueReference ||
                result_type.kind == CXType_RValueReference) {
                CXCursor parent = clang_getCursorSemanticParent(cursor);
                std::string class_name = ClangUtils::get_cursor_spelling(parent);
                std::string file = ClangUtils::get_file_path(cursor);
                
                if (!class_name.empty() && !ClangUtils::is_system_header(file)) {
                    LintIssue issue;
                    issue.type = CheckType::SINGLETON_PATTERN;
                    issue.severity = Severity::INFO;
                    issue.checker_name = "singleton";
                    issue.name = method_name;
                    issue.type_str = class_name;
                    issue.file = file;
                    issue.line = ClangUtils::get_line_number(cursor);
                    issue.column = ClangUtils::get_column_number(cursor);
                    issue.description = "Singleton pattern detected";
                    issue.suggestion = class_name + "::" + method_name + "()";
                    issue.fixable = false;
                    result.add_issue(issue);
                }
            }
        }
    }
    
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
            auto* res = static_cast<LintResult*>(data);
            auto* self = reinterpret_cast<SingletonChecker*>(
                reinterpret_cast<uintptr_t*>(data)[1]);
            self->visit_cursor(c, *res);
            return CXChildVisit_Continue;
        },
        &result);
}

}  // namespace lint
}  // namespace codelint