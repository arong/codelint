#include "lint/checkers/global_checker.h"

namespace codelint {
namespace lint {

LintResult GlobalChecker::check(const std::string& filepath) {
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

void GlobalChecker::visit_cursor(CXCursor cursor, LintResult& result) {
    if (clang_isDeclaration(clang_getCursorKind(cursor))) {
        CXCursorKind kind = clang_getCursorKind(cursor);
        CXCursor parent = clang_getCursorSemanticParent(cursor);
        
        if ((kind == CXCursor_VarDecl) &&
            (clang_getCursorKind(parent) == CXCursor_TranslationUnit)) {
            
            std::string name = ClangUtils::get_cursor_spelling(cursor);
            std::string type = ClangUtils::get_type_spelling(cursor);
            std::string file = ClangUtils::get_file_path(cursor);
            
            if (!name.empty() && !type.empty() && !ClangUtils::is_system_header(file)) {
                LintIssue issue;
                issue.type = CheckType::GLOBAL_VARIABLE;
                issue.severity = Severity::WARNING;
                issue.checker_name = "global";
                issue.name = name;
                issue.type_str = type;
                issue.file = file;
                issue.line = ClangUtils::get_line_number(cursor);
                issue.column = ClangUtils::get_column_number(cursor);
                issue.description = "Global variable detected";
                issue.suggestion = "Consider using a singleton or dependency injection";
                issue.fixable = false;
                result.add_issue(issue);
            }
        }
    }
    
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
            auto* res = static_cast<LintResult*>(data);
            auto* self = reinterpret_cast<GlobalChecker*>(
                reinterpret_cast<uintptr_t*>(data)[1]);
            self->visit_cursor(c, *res);
            return CXChildVisit_Continue;
        },
        &result);
}

}  // namespace lint
}  // namespace codelint