#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "commands.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

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
      Document doc;
      doc.SetObject();
      auto& allocator = doc.GetAllocator();
      doc.AddMember("command", "find_singleton", allocator);
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
    find_singletons_in_file(file, singletons);
  }

  if (g_opts.output_json) {
    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    doc.AddMember("command", "find_singleton", allocator);
    doc.AddMember("path", StringRef(g_opts.path.c_str()), allocator);
    doc.AddMember("status", "success", allocator);
    doc.AddMember("count", static_cast<int>(singletons.size()), allocator);

    Value singletons_array(kArrayType);
    for (const auto& s : singletons) {
      Value obj(kObjectType);
      obj.AddMember("name", StringRef(s.name.c_str()), allocator);
      obj.AddMember("className", StringRef(s.className.c_str()), allocator);
      obj.AddMember("file", StringRef(s.file.c_str()), allocator);
      obj.AddMember("line", s.line, allocator);
      singletons_array.PushBack(obj, allocator);
    }
    doc.AddMember("singletons", singletons_array, allocator);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::cout << buffer.GetString() << std::endl;
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