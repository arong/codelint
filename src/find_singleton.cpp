#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "commands.h"
#include "utils.h"

struct SingletonInfo {
  std::string name;
  std::string file;
  int line;
  std::string className;
};

bool find_singletons_in_file(const std::string& filepath,
                             std::vector<SingletonInfo>& singletons) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return false;
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  std::regex singleton_pattern(
      R"((\w+)\s*&\s*(?:instance|getInstance)\s*\(\)\s*\{[\s\n]*static\s+(\w+)\s+(\w+)[\s\n;]+)",
      std::regex::ECMAScript);

  std::smatch match;
  std::string::const_iterator search_start(content.cbegin());
  int current_pos = 1;
  int line_count = 1;

  while (std::regex_search(search_start, content.cend(), match,
                           singleton_pattern)) {
    std::string matched_text = match.prefix().str();
    line_count += std::count(matched_text.begin(), matched_text.end(), '\n');

    SingletonInfo info;
    info.name = "instance";
    info.file = filepath;
    info.line = line_count;
    info.className = match[1];
    singletons.push_back(info);

    search_start = match[0].second;
  }

  file.close();
  return true;
}

void find_singleton() {
  std::vector<SingletonInfo> singletons;

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
      std::cout << "{\"command\":\"find_singleton\",\"path\":\"" << g_opts.path
                << "\",\"status\":\"error\",\"message\":\"" << e.what() << "\"}"
                << std::endl;
    } else {
      std::cerr << "Error: " << e.what() << std::endl;
    }
    return;
  }

  for (const auto& file : cpp_files) {
    find_singletons_in_file(file, singletons);
  }

  if (g_opts.output_json) {
    std::cout << "{\"command\":\"find_singleton\",\"path\":\"" << g_opts.path
              << "\",\"status\":\"success\",\"count\":" << singletons.size()
              << ",\"singletons\":[";
    for (size_t i = 0; i < singletons.size(); ++i) {
      const auto& s = singletons[i];
      std::cout << "{\"name\":\"" << escape_json(s.name)
                << "\",\"className\":\"" << escape_json(s.className)
                << "\",\"file\":\"" << escape_json(s.file)
                << "\",\"line\":" << s.line << "}";
      if (i < singletons.size() - 1) {
        std::cout << ",";
      }
    }
    std::cout << "]}" << std::endl;
  } else {
    std::cout << "Finding singleton instances in: " << g_opts.path << std::endl;
    std::cout << "Status: success" << std::endl;
    std::cout << "Found " << singletons.size() << " singleton instance(s)"
              << std::endl;
    for (const auto& s : singletons) {
      std::cout << "  - " << s.className << "::" << s.name << "() at " << s.file
                << ":" << s.line << std::endl;
    }
  }
}