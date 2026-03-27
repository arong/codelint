#include <clang-c/Index.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
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
                          "-I/usr/include",
                          "-I/usr/local/include"};
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
    std::set<std::string> reported_vars_;
    std::map<std::string, std::vector<std::string>> file_contents_;

    bool has_explicit_initializer(const std::string& file, int line, const std::string& var_name) {
      if (file_contents_.find(file) == file_contents_.end()) {
        std::ifstream f(file);
        if (!f.is_open()) return false;
        std::vector<std::string> lines;
        std::string l;
        while (std::getline(f, l)) {
          lines.push_back(l);
        }
        file_contents_[file] = lines;
      }

      const auto& lines = file_contents_[file];
      if (line <= 0 || line > (int)lines.size()) return false;

      const std::string& line_content = lines[line - 1];
      size_t var_pos = line_content.find(var_name);
      if (var_pos == std::string::npos) return false;

      size_t var_end = var_pos + var_name.length();

      while (var_end < line_content.length() &&
             (line_content[var_end] == ' ' || line_content[var_end] == '\t')) {
        var_end++;
      }

      while (var_end < line_content.length() && line_content[var_end] == '[') {
        int bracket_count = 1;
        size_t pos = var_end + 1;
        while (pos < line_content.length() && bracket_count > 0) {
          if (line_content[pos] == '[') bracket_count++;
          else if (line_content[pos] == ']') bracket_count--;
          pos++;
        }
        var_end = pos;
      }

      while (var_end < line_content.length() &&
             (line_content[var_end] == ' ' || line_content[var_end] == '\t')) {
        var_end++;
      }

      if (var_end >= line_content.length()) return false;
      char next_char = line_content[var_end];
      return (next_char == '{' || next_char == '(' || next_char == '=');
    }

    void visit_cursor(CXCursor cursor) {
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl) {
      std::string file = get_file_location(cursor);
      int line = get_line_number(cursor);
      std::string name = get_cursor_spelling(cursor);
      std::string key = file + ":" + std::to_string(line) + ":" + name;

      if (kind == CXCursor_VarDecl) {
        if (reported_vars_.insert(key).second) {
          check_variable(cursor);
        }
      } else {
        check_variable(cursor);
      }
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* checker = static_cast<InitChecker*>(data);
          checker->visit_cursor(c);
          return CXChildVisit_Recurse;
        },
        this);
  }

  void check_variable(CXCursor cursor) {
    std::string var_name = get_cursor_spelling(cursor);
    std::string type_str = get_type_spelling(cursor);

    if (var_name.empty() || type_str.empty()) {
      return;
    }

    // Skip union members - they share memory and shouldn't be initialized individually
    CXCursor parent = clang_getCursorSemanticParent(cursor);
    if (clang_getCursorKind(parent) == CXCursor_UnionDecl) {
      return;
    }

    CX_StorageClass storage = clang_Cursor_getStorageClass(cursor);
    if (storage == CX_SC_Extern) {
      return;
    }

    // Skip enum class types where 0 is not a valid enumerator value
    // Using {} would initialize to 0, which may not be a valid enum value
    CXType var_type = clang_getCursorType(cursor);
    CXCursor type_decl = clang_getTypeDeclaration(var_type);
    if (clang_getCursorKind(type_decl) == CXCursor_EnumDecl) {
      // Check if this is an enum class (scoped enum) and if 0 is not a valid value
      if (clang_EnumDecl_isScoped(type_decl)) {
        bool has_zero_value = false;
        clang_visitChildren(type_decl, [](CXCursor c, CXCursor parent, CXClientData data) {
          if (clang_getCursorKind(c) == CXCursor_EnumConstantDecl) {
            auto* has_zero = static_cast<bool*>(data);
            if (clang_getEnumConstantDeclValue(c) == 0) {
              *has_zero = true;
            }
          }
          return CXChildVisit_Continue;
        }, &has_zero_value);
        if (!has_zero_value) {
          return;
        }
      }
    }

    std::string file = get_file_location(cursor);
    int line = get_line_number(cursor);

    if (is_system_header(file)) {
      return;
    }

    CXCursorKind kind = clang_getCursorKind(cursor);
    bool has_explicit = has_explicit_initializer(file, line, var_name);

    if (!has_explicit) {
      check_uninitialized(var_name, type_str, file, line);
      return;
    }

    CXCursor init_cursor = clang_Cursor_getVarDeclInitializer(cursor);

    if (clang_Cursor_isNull(init_cursor)) {
      return;
    }

    if (kind == CXCursor_FieldDecl) {
      check_initializer(cursor, init_cursor, var_name, type_str, file, line);
    } else {
      check_initializer(cursor, init_cursor, var_name, type_str, file, line);
    }
  }

  void check_uninitialized(const std::string& var_name,
                           const std::string& type_str, const std::string& file,
                           int line) {
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
    CXCursorKind init_kind = clang_getCursorKind(init_cursor);

    // Brace initialization: CXCursor_InitListExpr or empty constructor call
    bool is_brace_init = (init_kind == CXCursor_InitListExpr);
    
    // For non-class types with {}, init_cursor might be a CallExpr
    // Check if it's a default constructor call (no arguments)
    if (!is_brace_init && init_kind == CXCursor_CallExpr) {
      // Check if this is a default constructor call (empty parens/braces)
      unsigned num_args = clang_Cursor_getNumArguments(init_cursor);
      if (num_args == 0) {
        is_brace_init = true;
      }
    }

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
    std::string init_value = get_init_value(init_cursor);
    
    // Skip if init_value contains braces (already using {} initialization)
    if (init_value.find('{') != std::string::npos || 
        init_value.find('}') != std::string::npos) {
      return;
    }
    
    // Skip if init_value is empty or contains only whitespace
    if (init_value.empty() || 
        init_value.find_first_not_of(" \t\n\r") == std::string::npos) {
      return;
    }
    
    // Skip if init_value equals variable name (default initialization)
    if (init_value == var_name) {
      return;
    }

    InitIssue issue;
    issue.type = InitIssueType::USE_EQUALS_INIT;
    issue.name = var_name;
    issue.type_str = type_str;
    issue.file = file;
    issue.line = line;
    issue.description = "Variable should use '{}' syntax for initialization";

    bool is_unsigned = is_unsigned_type(type_str);

    // Handle unsigned suffix for numeric values
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

  bool is_builtin_type(const std::string& type_str) {
    // Check for primitive types (int, char, float, bool, etc.)
    // Non-builtin types like std::vector, std::map, MyClass have constructors
    if (type_str.find("std::") != std::string::npos) {
      return false;
    }
    if (type_str.find("struct ") != std::string::npos ||
        type_str.find("class ") != std::string::npos) {
      return false;
    }
    // Check for primitive type keywords
    static const char* builtin_types[] = {
      "int", "char", "bool", "float", "double", "void",
      "short", "long", "signed", "unsigned", "wchar_t", "char16_t", "char32_t"
    };
    for (const auto* t : builtin_types) {
      if (type_str.find(t) != std::string::npos) {
        return true;
      }
    }
    return false;
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
    const char* filename_cstr = clang_getCString(filename);

    std::string result;
    if (filename_cstr) {
      result = filename_cstr;
    }
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
      clang_disposeString(token_spelling);
    }

    clang_disposeTokens(tu, tokens, num_tokens);
    return result;
  }
};

void check_init() {
  std::vector<InitIssue> issues;

  std::vector<std::string> cpp_files;

  // If specific files are provided, use them
  if (!g_check_init_opts.files.empty()) {
    for (const auto& file : g_check_init_opts.files) {
      // Verify files exist and canonicalize paths
      if (!std::filesystem::exists(file)) {
        std::cerr << "Error: File not found: " << file << std::endl;
        return;
      }
      try {
        cpp_files.push_back(std::filesystem::canonical(file).string());
      } catch (...) {
        cpp_files.push_back(file);
      }
    }
  } else {
    // Read compile_commands.json to get all compiled files
    std::filesystem::path compile_commands_path =
        std::filesystem::path(g_opts.path) / "compile_commands.json";

    if (!std::filesystem::exists(compile_commands_path)) {
      std::cerr << "Error: compile_commands.json not found in: " << g_opts.path
                << std::endl;
      return;
    }

    try {
      std::ifstream file(compile_commands_path);
      if (!file.is_open()) {
        std::cerr << "Error: Failed to open compile_commands.json: "
                  << compile_commands_path << std::endl;
        return;
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string content = buffer.str();
      file.close();

      Document doc;
      doc.Parse(content.c_str());

      if (doc.HasParseError()) {
        std::cerr << "Error: Failed to parse compile_commands.json"
                  << std::endl;
        return;
      }

      if (!doc.IsArray()) {
        std::cerr << "Error: compile_commands.json should be an array"
                  << std::endl;
        return;
      }

      // Extract unique file paths
      std::set<std::string> file_set;
      for (const auto& entry : doc.GetArray()) {
        if (entry.IsObject() && entry.HasMember("file")) {
          std::string file_path = entry["file"].GetString();
          std::filesystem::path p(file_path);
          std::string ext = p.extension().string();
          if (ext == ".cpp" || ext == ".cc" || ext == ".c++" || ext == ".cxx" ||
              ext == ".h" || ext == ".hpp") {
            file_set.insert(file_path);
          }
        }
      }

      cpp_files.assign(file_set.begin(), file_set.end());

    } catch (const std::exception& e) {
      std::cerr << "Error reading compile_commands.json: " << e.what()
                << std::endl;
      return;
    }
  }

  if (cpp_files.empty()) {
    std::cerr << "Error: No C++ source files found to check" << std::endl;
    return;
  }

  InitChecker checker;
  for (const auto& file : cpp_files) {
    auto file_issues = checker.check_init(file);
    issues.insert(issues.end(), file_issues.begin(), file_issues.end());
  }

  // Apply fixes if requested
  if (g_opts.fix && !issues.empty()) {
    std::map<std::string, std::vector<InitIssue>> file_issues_map;
    for (const auto& issue : issues) {
      file_issues_map[issue.file].push_back(issue);
    }

    for (const auto& [file_path, issue_list] : file_issues_map) {
      std::ifstream input_file(file_path);
      if (!input_file.is_open()) {
        continue;
      }

      std::stringstream buffer;
      buffer << input_file.rdbuf();
      std::string content = buffer.str();
      input_file.close();

      std::map<int, std::vector<InitIssue>> line_issues;
      for (const auto& issue : issue_list) {
        line_issues[issue.line].push_back(issue);
      }

      std::vector<std::string> lines;
      std::stringstream ss(content);
      std::string line;
      while (std::getline(ss, line)) {
        lines.push_back(line);
      }

      for (auto& [line_num, line_issues_vec] : line_issues) {
        if (line_num > 0 && line_num <= (int)lines.size()) {
          int idx = line_num - 1;
          
          // Sort issues by variable name position in line (right to left)
          // to avoid position shifts when inserting {}
          std::vector<std::pair<size_t, InitIssue>> sorted_issues;
          for (const auto& issue : line_issues_vec) {
            size_t var_pos = lines[idx].find(issue.name);
            if (var_pos != std::string::npos) {
              sorted_issues.push_back({var_pos, issue});
            }
          }
          std::sort(sorted_issues.begin(), sorted_issues.end(), 
                    [](const auto& a, const auto& b) { return a.first > b.first; });
          
          for (const auto& [var_pos, issue] : sorted_issues) {
            if (issue.type == InitIssueType::UNINITIALIZED) {
              // Find variable name position and insert {} after it, preserving modifiers
              size_t var_end = var_pos + issue.name.length();
              // Skip whitespace after variable name
              while (var_end < lines[idx].length() &&
                     (lines[idx][var_end] == ' ' || lines[idx][var_end] == '\t')) {
                var_end++;
              }
              // For arrays, skip array dimensions [...] and insert {} after ]
              while (var_end < lines[idx].length() && lines[idx][var_end] == '[') {
                // Find matching closing bracket for this dimension
                int bracket_count = 1;
                size_t pos = var_end + 1;
                while (pos < lines[idx].length() && bracket_count > 0) {
                  if (lines[idx][pos] == '[') bracket_count++;
                  else if (lines[idx][pos] == ']') bracket_count--;
                  pos++;
                }
                var_end = pos;
              }
              // Handle comma or semicolon
              if (var_end < lines[idx].length() && 
                  (lines[idx][var_end] == ';' || lines[idx][var_end] == ',')) {
                lines[idx].insert(var_end, "{}");
              }
            } else if (issue.type == InitIssueType::USE_EQUALS_INIT) {
              size_t pos = lines[idx].find("=");
              if (pos != std::string::npos) {
                // Handle case: Type name = value;
                size_t comment_pos = lines[idx].find("//");
                if (comment_pos == std::string::npos || comment_pos > pos) {
                  size_t end_pos = lines[idx].find(";", pos);
                  if (end_pos != std::string::npos) {
                    bool has_unsigned_issue = false;
                    for (const auto& check_issue : line_issues_vec) {
                      if (check_issue.type ==
                              InitIssueType::UNSIGNED_WITHOUT_SUFFIX &&
                          check_issue.name == issue.name) {
                        has_unsigned_issue = true;
                        break;
                      }
                    }

                    // Find the start of the replacement (skip all spaces before
                    // '=')
                    size_t replace_start = pos;
                    while (replace_start > 0 &&
                           (lines[idx][replace_start - 1] == ' ' ||
                            lines[idx][replace_start - 1] == '\t')) {
                      replace_start--;
                    }

                    // Get the full initialization part after =
                    std::string init_part =
                        lines[idx].substr(pos + 1, end_pos - pos - 1);

                    // Find the first non-whitespace character after =
                    size_t val_start = init_part.find_first_not_of(" \t");
                    if (val_start != std::string::npos) {
                      init_part = init_part.substr(val_start);
                    }

                    // Trim trailing whitespace
                    size_t val_end = init_part.find_last_not_of(" \t");
                    if (val_end != std::string::npos) {
                      init_part = init_part.substr(0, val_end + 1);
                    }

                    // Check if init_part already contains braces (like {1, 2,
                    // 3})
                    if (init_part.size() >= 2 && init_part.front() == '{' &&
                        init_part.back() == '}') {
                      // Already has braces, just replace =...; with init_part;
                      lines[idx].replace(replace_start,
                                         end_pos - replace_start + 1,
                                         init_part + ";");
                    } else {
                      // Need to add braces
                      std::string replacement;
                      if (has_unsigned_issue) {
                        replacement = "{" + init_part + "U};";
                      } else {
                        replacement = "{" + init_part + "};";
                      }
                      lines[idx].replace(replace_start,
                                         end_pos - replace_start + 1,
                                         replacement);
                    }
                  }
                }
              } else {
                // Handle case: Type name; or Type name(args); (no = sign)
                // Insert {} after variable name, before semicolon
                size_t var_pos = lines[idx].find(issue.name);
                if (var_pos != std::string::npos) {
                  size_t var_end = var_pos + issue.name.length();
                  // Skip whitespace after variable name
                  while (var_end < lines[idx].length() &&
                         (lines[idx][var_end] == ' ' || lines[idx][var_end] == '\t')) {
                    var_end++;
                  }
                  // Check for constructor call ( or just semicolon
                  if (var_end < lines[idx].length() && 
                      (lines[idx][var_end] == '(' || lines[idx][var_end] == ';')) {
                    if (lines[idx][var_end] == '(') {
                      // Replace (...) with {}
                      size_t close_paren = lines[idx].find(")", var_end);
                      if (close_paren != std::string::npos) {
                        std::string init_value = lines[idx].substr(var_end + 1, close_paren - var_end - 1);
                        // Trim whitespace
                        size_t val_start = init_value.find_first_not_of(" \t");
                        if (val_start != std::string::npos) {
                          init_value = init_value.substr(val_start);
                        }
                        size_t val_end = init_value.find_last_not_of(" \t");
                        if (val_end != std::string::npos) {
                          init_value = init_value.substr(0, val_end + 1);
                        }
                        // Replace (...) with {value} or {}
                        if (init_value.empty()) {
                          lines[idx].replace(var_end, close_paren - var_end + 1, "{}");
                        } else {
                          lines[idx].replace(var_end, close_paren - var_end + 1, "{" + init_value + "}");
                        }
                      }
                    } else {
                      // Insert {} before semicolon
                      lines[idx].insert(var_end, "{}");
                    }
                  }
                }
              }
            }
          }
        }
      }

      if (g_opts.inplace) {
        std::ofstream output_file(file_path);
        if (!output_file.is_open()) {
          std::cerr << "Failed to write to file: " << file_path << std::endl;
          continue;
        }

        for (const auto& l : lines) {
          output_file << l << "\n";
        }
        output_file.close();
      } else {
        // Only output the fixed code, no extra headers or summaries
        for (const auto& l : lines) {
          std::cout << l << "\n";
        }
      }
    }

    if (g_opts.inplace) {
      std::cout << "\nFixed " << file_issues_map.size() << " file(s) in-place."
                << std::endl;
    }
    return;
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
      std::cout << "  Variable: " << issue.name << std::endl;
      std::cout << "  Location: " << issue.file << ":" << issue.line
                << std::endl;
      std::cout << "  Description: " << issue.description << std::endl;
      std::cout << "  Suggestion: " << issue.suggestion << std::endl;
    }
  }
}