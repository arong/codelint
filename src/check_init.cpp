#include <clang-c/Index.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "commands.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

enum class InitIssueType {
  UNINITIALIZED,
  USE_EQUALS_INIT,
  UNSIGNED_WITHOUT_SUFFIX
};

struct InitIssue {
  InitIssueType type;
  std::string name;
  std::string type_str;
  std::string file;
  int line;
  std::string suggestion;
  std::string description;
};

std::string get_issue_type_name(InitIssueType type) {
  switch (type) {
    case InitIssueType::UNINITIALIZED:
      return "uninitialized";
    case InitIssueType::USE_EQUALS_INIT:
      return "use_equals_init";
    case InitIssueType::UNSIGNED_WITHOUT_SUFFIX:
      return "unsigned_without_suffix";
    default:
      return "unknown";
  }
}

class InitChecker {
 public:
  std::vector<InitIssue> check_init(const std::string& filepath) {
    issues_.clear();

    CXIndex index = clang_createIndex(0, 0);
    if (!index) {
      std::cerr << "Failed to create clang index" << std::endl;
      return issues_;
    }

    const char* args[] = {"-std=c++17",
                          "-x",
                          "c++",
                          "-I/usr/include/c++/13",
                          "-I/usr/include/x86_64-linux-gnu/c++/13",
                          "-I/usr/include"};
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, filepath.c_str(), args, 6, nullptr, 0, CXTranslationUnit_None);

    if (!tu) {
      std::cerr << "Failed to parse translation unit: " << filepath
                << std::endl;
      clang_disposeIndex(index);
      return issues_;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* checker = static_cast<InitChecker*>(data);
          checker->visit_cursor(c);
          return CXChildVisit_Continue;
        },
        this);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return issues_;
  }

 private:
  std::vector<InitIssue> issues_;
  bool in_function_ = false;

  void visit_cursor(CXCursor cursor) {
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
      in_function_ = true;
      clang_visitChildren(
          cursor,
          [](CXCursor c, CXCursor parent, CXClientData data) {
            auto* checker = static_cast<InitChecker*>(data);
            checker->visit_function_body(c);
            return CXChildVisit_Continue;
          },
          this);
      in_function_ = false;
    } else {
      clang_visitChildren(
          cursor,
          [](CXCursor c, CXCursor parent, CXClientData data) {
            auto* checker = static_cast<InitChecker*>(data);
            checker->visit_cursor(c);
            return CXChildVisit_Continue;
          },
          this);
    }
  }

  void visit_function_body(CXCursor cursor) {
    if (clang_getCursorKind(cursor) == CXCursor_VarDecl) {
      check_variable(cursor);
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* checker = static_cast<InitChecker*>(data);
          checker->visit_function_body(c);
          return CXChildVisit_Continue;
        },
        this);
  }

  void check_variable(CXCursor cursor) {
    std::string var_name = get_cursor_spelling(cursor);
    std::string type_str = get_type_spelling(cursor);

    if (var_name.empty() || type_str.empty()) {
      return;
    }

    std::string file = get_file_location(cursor);
    int line = get_line_number(cursor);

    if (is_system_header(file)) {
      return;
    }

    CXCursor init_cursor = clang_Cursor_getVarDeclInitializer(cursor);

    if (clang_Cursor_isNull(init_cursor)) {
      check_uninitialized(var_name, type_str, file, line);
    } else {
      check_initializer(cursor, init_cursor, var_name, type_str, file, line);
    }
  }

  void check_uninitialized(const std::string& var_name,
                           const std::string& type_str, const std::string& file,
                           int line) {
    if (!is_builtin_type(type_str)) {
      return;
    }

    InitIssue issue;
    issue.type = InitIssueType::UNINITIALIZED;
    issue.name = var_name;
    issue.type_str = type_str;
    issue.file = file;
    issue.line = line;
    issue.description = "Variable is not explicitly initialized";
    issue.suggestion = type_str + " " + var_name + "{};";
    issues_.push_back(issue);
  }

  void check_initializer(CXCursor var_cursor, CXCursor init_cursor,
                         const std::string& var_name,
                         const std::string& type_str, const std::string& file,
                         int line) {
    if (!is_builtin_type(type_str)) {
      return;
    }

    CXCursorKind init_kind = clang_getCursorKind(init_cursor);

    bool is_brace_init = (init_kind == CXCursor_InitListExpr);

    if (!is_brace_init) {
      check_equals_init(var_name, type_str, file, line, init_cursor);
    }

    bool is_unsigned = is_unsigned_type(type_str);
    if (is_unsigned && !is_brace_init) {
      check_unsigned_suffix(var_name, type_str, file, line, init_cursor);
    }
  }

  void check_equals_init(const std::string& var_name,
                         const std::string& type_str, const std::string& file,
                         int line, CXCursor init_cursor) {
    InitIssue issue;
    issue.type = InitIssueType::USE_EQUALS_INIT;
    issue.name = var_name;
    issue.type_str = type_str;
    issue.file = file;
    issue.line = line;
    issue.description = "Variable initialized with '=' should use '{}' syntax";

    std::string init_value = get_init_value(init_cursor);
    bool is_unsigned = is_unsigned_type(type_str);

    if (is_unsigned && has_digit_value(init_value) &&
        !has_unsigned_suffix(init_value)) {
      issue.suggestion = type_str + " " + var_name + "{" + init_value + "U};";
    } else {
      issue.suggestion = type_str + " " + var_name + "{" + init_value + "};";
    }

    issues_.push_back(issue);
  }

  void check_unsigned_suffix(const std::string& var_name,
                             const std::string& type_str,
                             const std::string& file, int line,
                             CXCursor init_cursor) {
    std::string init_value = get_init_value(init_cursor);

    if (has_digit_value(init_value) && !has_unsigned_suffix(init_value)) {
      InitIssue issue;
      issue.type = InitIssueType::UNSIGNED_WITHOUT_SUFFIX;
      issue.name = var_name;
      issue.type_str = type_str;
      issue.file = file;
      issue.line = line;
      issue.description = "Unsigned integer should have 'U' or 'UL' suffix";
      issue.suggestion = "";
      issues_.push_back(issue);
    }
  }

  bool is_builtin_type(const std::string& type_str) {
    std::vector<std::string> builtin_types = {"int",
                                              "unsigned",
                                              "char",
                                              "float",
                                              "double",
                                              "bool",
                                              "unsigned int",
                                              "unsigned char",
                                              "short",
                                              "long",
                                              "unsigned long",
                                              "long long",
                                              "unsigned long long"};

    for (const auto& builtin : builtin_types) {
      if (type_str.find(builtin) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  bool is_unsigned_type(const std::string& type_str) {
    return type_str.find("unsigned") != std::string::npos;
  }

  bool has_digit_value(const std::string& value) {
    return std::any_of(value.begin(), value.end(), ::isdigit);
  }

  bool has_unsigned_suffix(const std::string& value) {
    return value.find('U') != std::string::npos ||
           value.find('u') != std::string::npos;
  }

  bool is_system_header(const std::string& file) {
    return file.find("/usr/include/") == 0 || file.find("/usr/lib/") == 0;
  }

  std::string get_cursor_spelling(CXCursor cursor) {
    CXString spelling = clang_getCursorSpelling(cursor);
    std::string result = clang_getCString(spelling);
    clang_disposeString(spelling);
    return result;
  }

  std::string get_type_spelling(CXCursor cursor) {
    CXType type = clang_getCursorType(cursor);
    CXString spelling = clang_getTypeSpelling(type);
    std::string result = clang_getCString(spelling);
    clang_disposeString(spelling);
    return result;
  }

  std::string get_file_location(CXCursor cursor) {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line, column, offset;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);

    CXString filename = clang_getFileName(file);
    std::string result = clang_getCString(filename);
    clang_disposeString(filename);

    return result;
  }

  int get_line_number(CXCursor cursor) {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line, column, offset;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);
    return static_cast<int>(line);
  }

  std::string get_init_value(CXCursor cursor) {
    std::string result;
    CXSourceRange range = clang_getCursorExtent(cursor);

    CXToken* tokens = nullptr;
    unsigned num_tokens = 0;
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    clang_tokenize(tu, range, &tokens, &num_tokens);

    for (unsigned i = 0; i < num_tokens; ++i) {
      CXString token_spelling = clang_getTokenSpelling(tu, tokens[i]);
      result += clang_getCString(token_spelling);
      result += " ";
      clang_disposeString(token_spelling);
    }

    clang_disposeTokens(tu, tokens, num_tokens);
    return result;
  }
};

void check_init() {
  std::vector<InitIssue> issues;

  std::filesystem::path path(g_opts.path);
  std::vector<std::string> cpp_files;

  try {
    if (std::filesystem::is_directory(path)) {
      for (const auto& entry :
           std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
          std::string ext = entry.path().extension().string();
          if (ext == ".cpp" || ext == ".cc" || ext == ".c++" || ext == ".cxx" ||
              ext == ".h" || ext == ".hpp") {
            cpp_files.push_back(entry.path().string());
          }
        }
      }
    } else if (std::filesystem::is_regular_file(path)) {
      cpp_files.push_back(path.string());
    }
  } catch (const std::filesystem::filesystem_error& e) {
    if (g_opts.output_json) {
      Document doc;
      doc.SetObject();
      auto& allocator = doc.GetAllocator();
      doc.AddMember("command", "check_init", allocator);
      doc.AddMember("path", StringRef(g_opts.path.c_str()), allocator);
      doc.AddMember("status", "error", allocator);
      doc.AddMember("message", StringRef(e.what()), allocator);

      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      doc.Accept(writer);
      std::cout << buffer.GetString() << std::endl;
    } else {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return;
  }

  InitChecker checker;
  for (const auto& file : cpp_files) {
    auto file_issues = checker.check_init(file);
    issues.insert(issues.end(), file_issues.begin(), file_issues.end());
  }

  if (g_opts.output_json) {
    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    doc.AddMember("command", "check_init", allocator);
    doc.AddMember("path", StringRef(g_opts.path.c_str()), allocator);
    doc.AddMember("status", "success", allocator);
    doc.AddMember("count", static_cast<int>(issues.size()), allocator);

    Value issues_array(kArrayType);
    for (const auto& issue : issues) {
      Value obj(kObjectType);
      obj.AddMember("type", StringRef(get_issue_type_name(issue.type).c_str()),
                    allocator);
      obj.AddMember("name", StringRef(issue.name.c_str()), allocator);
      obj.AddMember("varType", StringRef(issue.type_str.c_str()), allocator);
      obj.AddMember("file", StringRef(issue.file.c_str()), allocator);
      obj.AddMember("line", issue.line, allocator);
      obj.AddMember("description", StringRef(issue.description.c_str()),
                    allocator);
      obj.AddMember("suggestion", StringRef(issue.suggestion.c_str()),
                    allocator);
      issues_array.PushBack(obj, allocator);
    }
    doc.AddMember("issues", issues_array, allocator);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::cout << buffer.GetString() << std::endl;
  } else {
    std::cout << "Checking initialization in: " << g_opts.path << std::endl;
    std::cout << "Status: success" << std::endl;
    std::cout << "Found " << issues.size() << " initialization issue(s)"
              << std::endl;
    for (const auto& issue : issues) {
      std::cout << "\n  [Issue: " << get_issue_type_name(issue.type) << "]"
                << std::endl;
      std::cout << "  Variable: " << issue.name << " (" << issue.type_str << ")"
                << std::endl;
      std::cout << "  Location: " << issue.file << ":" << issue.line
                << std::endl;
      std::cout << "  Description: " << issue.description << std::endl;
      std::cout << "  Suggestion: " << issue.suggestion << std::endl;
    }
  }
}