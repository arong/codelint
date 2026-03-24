#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
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

bool check_init_in_file(const std::string& filepath,
                        std::vector<InitIssue>& issues) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  int line_num = 0;
  bool in_function = false;
  bool in_class = false;

  std::regex var_pattern(
      R"(int|unsigned|char|float|double|bool|size_t|string|vector)",
      std::regex::ECMAScript);

  while (std::getline(file, line)) {
    line_num++;

    if (line.find("{") != std::string::npos) {
      if (line.find("class") == std::string::npos &&
          line.find("struct") == std::string::npos &&
          line.find("namespace") == std::string::npos) {
        in_function = true;
      } else if (line.find("class") != std::string::npos ||
                 line.find("struct") != std::string::npos) {
        in_class = true;
      }
    }

    if (line.find("}") != std::string::npos) {
      in_function = false;
      in_class = false;
    }

    if (!in_class && in_function && line.find(';') != std::string::npos) {
      size_t equals_pos = line.find('=');
      size_t semicolon_pos = line.find(';');

      if (equals_pos != std::string::npos && equals_pos < semicolon_pos) {
        std::string type_part = line.substr(0, equals_pos);
        std::string init_part =
            line.substr(equals_pos + 1, semicolon_pos - equals_pos - 1);

        type_part.erase(0, type_part.find_first_not_of(" \t"));
        type_part.erase(type_part.find_last_not_of(" \t") + 1);

        init_part.erase(0, init_part.find_first_not_of(" \t"));
        init_part.erase(init_part.find_last_not_of(" \t") + 1);

        size_t last_space = type_part.rfind(' ');
        if (last_space != std::string::npos) {
          std::string type_str = type_part.substr(0, last_space);
          std::string var_name = type_part.substr(last_space + 1);

          if (!var_name.empty() && !type_str.empty() &&
              std::regex_search(type_str, var_pattern)) {
            bool is_unsigned = type_str.find("unsigned") != std::string::npos;
            bool has_brace_init = init_part.find('{') != std::string::npos;
            bool has_equals_init = !has_brace_init;

            std::string clean_init;
            if (has_equals_init) {
              clean_init = init_part;
              size_t comment_pos = clean_init.find("//");
              if (comment_pos != std::string::npos) {
                clean_init = clean_init.substr(0, comment_pos);
              }
              clean_init.erase(0, clean_init.find_first_not_of(" \t"));
              clean_init.erase(clean_init.find_last_not_of(" \t") + 1);
            }

            if (!has_brace_init) {
              InitIssue issue;
              issue.type = InitIssueType::USE_EQUALS_INIT;
              issue.name = var_name;
              issue.type_str = type_str;
              issue.file = filepath;
              issue.line = line_num;
              issue.description =
                  "Variable initialized with '=' should use '{}' syntax";

              if (is_unsigned && clean_init.find('U') == std::string::npos &&
                  std::any_of(clean_init.begin(), clean_init.end(),
                              ::isdigit)) {
                issue.suggestion =
                    type_str + " " + var_name + "{" + clean_init + "U};";
              } else {
                issue.suggestion =
                    type_str + " " + var_name + "{" + clean_init + "};";
              }
              issues.push_back(issue);
            }

            if (is_unsigned && clean_init.find('U') == std::string::npos &&
                std::any_of(clean_init.begin(), clean_init.end(), ::isdigit)) {
              InitIssue issue;
              issue.type = InitIssueType::UNSIGNED_WITHOUT_SUFFIX;
              issue.name = var_name;
              issue.type_str = type_str;
              issue.file = filepath;
              issue.line = line_num;
              issue.description =
                  "Unsigned integer should have 'U' or 'UL' suffix";
              issues.push_back(issue);
            }
          }
        }
      } else {
        std::string type_part = line.substr(0, semicolon_pos);
        type_part.erase(0, type_part.find_first_not_of(" \t"));
        type_part.erase(type_part.find_last_not_of(" \t") + 1);

        size_t last_space = type_part.rfind(' ');
        if (last_space != std::string::npos) {
          std::string type_str = type_part.substr(0, last_space);
          std::string var_name = type_part.substr(last_space + 1);

          if (!var_name.empty() && !type_str.empty() &&
              std::regex_search(type_str, var_pattern)) {
            InitIssue issue;
            issue.type = InitIssueType::UNINITIALIZED;
            issue.name = var_name;
            issue.type_str = type_str;
            issue.file = filepath;
            issue.line = line_num;
            issue.description = "Variable is not explicitly initialized";
            issue.suggestion = type_str + " " + var_name + "{};";
            issues.push_back(issue);
          }
        }
      }
    }
  }

  file.close();
  return true;
}

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

  for (const auto& file : cpp_files) {
    check_init_in_file(file, issues);
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