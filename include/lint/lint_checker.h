#pragma once

#include "lint_types.h"
#include <clang-c/Index.h>
#include <memory>
#include <vector>
#include <string>

namespace cndy {
namespace lint {

class ClangUtils {
public:
    static std::string get_cursor_spelling(CXCursor cursor) {
        CXString spelling = clang_getCursorSpelling(cursor);
        std::string result = clang_getCString(spelling);
        clang_disposeString(spelling);
        return result;
    }
    
    static std::string get_type_spelling(CXCursor cursor) {
        CXType type = clang_getCursorType(cursor);
        CXString spelling = clang_getTypeSpelling(type);
        std::string result = clang_getCString(spelling);
        clang_disposeString(spelling);
        return result;
    }
    
    static std::string get_file_path(CXCursor cursor) {
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        CXFile file;
        unsigned line, column, offset;
        clang_getExpansionLocation(loc, &file, &line, &column, &offset);
        
        CXString filename = clang_getFileName(file);
        std::string result = clang_getCString(filename) ? clang_getCString(filename) : "";
        clang_disposeString(filename);
        return result;
    }
    
    static int get_line_number(CXCursor cursor) {
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        CXFile file;
        unsigned line, column, offset;
        clang_getExpansionLocation(loc, &file, &line, &column, &offset);
        return static_cast<int>(line);
    }
    
    static int get_column_number(CXCursor cursor) {
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        CXFile file;
        unsigned line, column, offset;
        clang_getExpansionLocation(loc, &file, &line, &column, &offset);
        return static_cast<int>(column);
    }
    
    static std::string get_token_text(CXTranslationUnit tu, CXSourceRange range) {
        std::string result;
        CXToken* tokens = nullptr;
        unsigned num_tokens = 0;
        
        clang_tokenize(tu, range, &tokens, &num_tokens);
        for (unsigned i = 0; i < num_tokens; ++i) {
            CXString token_spelling = clang_getTokenSpelling(tu, tokens[i]);
            result += clang_getCString(token_spelling);
            clang_disposeString(token_spelling);
        }
        clang_disposeTokens(tu, tokens, num_tokens);
        
        return result;
    }
    
    static bool is_system_header(const std::string& file) {
        return file.find("/usr/include/") == 0 || 
               file.find("/usr/lib/") == 0 ||
               file.empty();
    }
    
    static std::string get_init_value(CXCursor init_cursor) {
        if (clang_Cursor_isNull(init_cursor)) return "";
        CXSourceRange range = clang_getCursorExtent(init_cursor);
        CXTranslationUnit tu = clang_Cursor_getTranslationUnit(init_cursor);
        return get_token_text(tu, range);
    }
};

class LintChecker {
public:
    virtual ~LintChecker() = default;
    
    virtual LintResult check(const std::string& filepath) = 0;
    
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<CheckType> provides() const = 0;
    
    virtual bool can_fix() const { return false; }
    virtual bool apply_fixes(const std::string& filepath, 
                            const std::vector<LintIssue>& issues,
                            std::string& modified_content) { 
        return false; 
    }
    
protected:
    CXTranslationUnit parse_file(CXIndex index, const std::string& filepath) const {
        const char* args[] = {
            "-std=c++17",
            "-x", "c++",
            "-I/usr/include/c++/13",
            "-I/usr/include/x86_64-linux-gnu/c++/13",
            "-I/usr/include",
            "-I/usr/local/include"
        };
        
        return clang_parseTranslationUnit(
            index, filepath.c_str(), args, 6, 
            nullptr, 0, CXTranslationUnit_None);
    }
};

class CheckerFactory {
public:
    static std::unique_ptr<LintChecker> create(const std::string& name);
    static std::vector<std::unique_ptr<LintChecker>> create_all();
    static std::vector<std::string> available_checkers();
};

}  // namespace lint
}  // namespace cndy