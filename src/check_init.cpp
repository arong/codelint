#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "commands.h"
#include "utils.h"

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
      std::cout << "{\"command\":\"check_init\",\"path\":\"" << g_opts.path
                << "\",\"status\":\"error\",\"message\":\"" << e.what() << "\"}"
                << std::endl;
    } else {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return;
  }

  for (const auto& file : cpp_files) {
    check_init_in_file(file, issues);
  }

  if (g_opts.output_json) {
    std::cout << "{\"command\":\"check_init\",\"path\":\"" << g_opts.path
              << "\",\"status\":\"success\",\"count\":" << issues.size()
              << ",\"issues\":[";
    for (size_t i = 0; i < issues.size(); ++i) {
      const auto& issue = issues[i];
      std::cout << "{\"type\":\"" << get_issue_type_name(issue.type)
                << "\",\"name\":\"" << escape_json(issue.name)
                << "\",\"varType\":\"" << escape_json(issue.type_str)
                << "\",\"file\":\"" << escape_json(issue.file)
                << "\",\"line\":" << issue.line << ",\"description\":\""
                << escape_json(issue.description) << "\",\"suggestion\":\""
                << escape_json(issue.suggestion) << "\"}";
      if (i < issues.size() - 1) {
        std::cout << ",";
      }
    }
    std::cout << "]}" << std::endl;
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