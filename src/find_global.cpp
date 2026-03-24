#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "commands.h"

struct GlobalVarInfo {
  std::string name;
  std::string type;
  std::string file;
  int line;
  bool isConst;
  std::string initValue;
};

std::string escape_json(const std::string& s) {
  std::string result;
  for (char c : s) {
    switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += c;
    }
  }
  return result;
}

bool find_globals_in_file(const std::string& filepath,
                          std::vector<GlobalVarInfo>& globals) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  int line_num = 0;
  bool in_namespace = false;
  bool in_class = false;

  std::regex global_var_pattern(
      R"(^\s*(static\s+)?(const\s+)?(thread_local\s+)?(volatile\s+)?(inline\s+)?([\w:<>]+)\s+(\w+)\s*(?:=\s*([^;]+))?;)",
      std::regex::ECMAScript | std::regex::optimize);

  while (std::getline(file, line)) {
    line_num++;

    if (line.find("namespace") != std::string::npos &&
        line.find('{') != std::string::npos) {
      in_namespace = true;
      continue;
    }
    if (line.find('}') != std::string::npos && in_namespace) {
      in_namespace = false;
      continue;
    }
    if (line.find("class") != std::string::npos ||
        line.find("struct") != std::string::npos) {
      if (line.find('{') != std::string::npos) {
        in_class = true;
      }
      continue;
    }
    if (line.find('}') != std::string::npos && in_class) {
      in_class = false;
      continue;
    }

    if (!in_class && !in_namespace) {
      std::smatch match;
      if (std::regex_search(line, match, global_var_pattern)) {
        GlobalVarInfo info;
        info.type = match[6];
        info.name = match[7];
        info.file = filepath;
        info.line = line_num;
        info.isConst = match[2].matched;
        info.initValue = match[8];

        std::regex type_pattern(
            R"(\b(?:int|float|double|char|bool|void|unsigned|signed|long|short|size_t|string|vector|map|array|auto)\b)");
        if (std::regex_search(info.type, type_pattern)) {
          globals.push_back(info);
        }
      }
    }
  }

  file.close();
  return true;
}

void find_global() {
  std::vector<GlobalVarInfo> globals;

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
      std::cout << "{\"command\":\"find_global\",\"path\":\"" << g_opts.path
                << "\",\"status\":\"error\",\"message\":\"" << e.what() << "\"}"
                << std::endl;
    } else {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return;
  }

  for (const auto& file : cpp_files) {
    find_globals_in_file(file, globals);
  }

  if (g_opts.output_json) {
    std::cout << "{\"command\":\"find_global\",\"path\":\"" << g_opts.path
              << "\",\"status\":\"success\",\"count\":" << globals.size()
              << ",\"globals\":[";
    for (size_t i = 0; i < globals.size(); ++i) {
      const auto& g = globals[i];
      std::cout << "{\"name\":\"" << escape_json(g.name) << "\",\"type\":\""
                << escape_json(g.type) << "\",\"file\":\""
                << escape_json(g.file) << "\",\"line\":" << g.line
                << ",\"isConst\":" << (g.isConst ? "true" : "false");
      if (!g.initValue.empty()) {
        std::cout << ",\"initValue\":\"" << escape_json(g.initValue) << "\"";
      }
      std::cout << "}";
      if (i < globals.size() - 1) {
        std::cout << ",";
      }
    }
    std::cout << "]}" << std::endl;
  } else {
    std::cout << "Finding global variables in: " << g_opts.path << std::endl;
    std::cout << "Status: success" << std::endl;
    std::cout << "Found " << globals.size() << " global variable(s)"
              << std::endl;
    for (const auto& g : globals) {
      std::cout << "  - " << g.type << " " << g.name;
      if (g.isConst) {
        std::cout << " (const)";
      }
      std::cout << " at " << g.file << ":" << g.line;
      if (!g.initValue.empty()) {
        std::cout << " = " << g.initValue;
      }
      std::cout << std::endl;
    }
  }
}