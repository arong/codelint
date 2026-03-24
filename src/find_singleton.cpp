#include <clang-c/Index.h>

#include <filesystem>
#include <iostream>
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

class SingletonFinder {
 public:
  std::vector<SingletonInfo> find_singletons(const std::string& filepath) {
    singletons_.clear();

    CXIndex index = clang_createIndex(0, 0);
    if (!index) {
      std::cerr << "Failed to create clang index" << std::endl;
      return singletons_;
    }

    const char* args[] = {"-std=c++17", "-x", "c++"};
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, filepath.c_str(), args, 3, nullptr, 0, CXTranslationUnit_None);

    if (!tu) {
      std::cerr << "Failed to parse translation unit: " << filepath
                << std::endl;
      clang_disposeIndex(index);
      return singletons_;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* finder = static_cast<SingletonFinder*>(data);
          finder->visit_cursor(c);
          return CXChildVisit_Continue;
        },
        this);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return singletons_;
  }

 private:
  std::vector<SingletonInfo> singletons_;

  void visit_cursor(CXCursor cursor) {
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_CXXMethod) {
      std::string method_name = get_cursor_spelling(cursor);

      if (method_name == "instance" || method_name == "getInstance") {
        CXType result_type = clang_getCursorResultType(cursor);

        if (result_type.kind == CXType_LValueReference ||
            result_type.kind == CXType_RValueReference) {
          CXCursor parent = clang_getCursorSemanticParent(cursor);
          std::string class_name = get_cursor_spelling(parent);

          if (!class_name.empty()) {
            SingletonInfo info;
            info.name = method_name;
            info.className = class_name;
            info.file = get_file_location(cursor);
            info.line = get_line_number(cursor);
            singletons_.push_back(info);
          }
        }
      }
    }

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* finder = static_cast<SingletonFinder*>(data);
          finder->visit_cursor(c);
          return CXChildVisit_Continue;
        },
        this);
  }

  std::string get_cursor_spelling(CXCursor cursor) {
    CXString spelling = clang_getCursorSpelling(cursor);
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
};

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

  SingletonFinder finder;
  for (const auto& file : cpp_files) {
    auto file_singletons = finder.find_singletons(file);
    singletons.insert(singletons.end(), file_singletons.begin(),
                      file_singletons.end());
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