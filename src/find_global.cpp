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

struct GlobalVarInfo {
  std::string name;
  std::string type;
  std::string file;
  int line;
  bool isConst;
  std::string initValue;
};

class GlobalVarFinder {
 public:
  std::vector<GlobalVarInfo> find_globals(const std::string& filepath) {
    globals_.clear();

    CXIndex index = clang_createIndex(0, 0);
    if (!index) {
      std::cerr << "Failed to create clang index" << std::endl;
      return globals_;
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
      return globals_;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData data) {
          auto* finder = static_cast<GlobalVarFinder*>(data);
          finder->visit_cursor(c);
          return CXChildVisit_Continue;
        },
        this);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return globals_;
  }

 private:
  std::vector<GlobalVarInfo> globals_;

  void visit_cursor(CXCursor cursor) {
    if (clang_isDeclaration(clang_getCursorKind(cursor))) {
      CXCursorKind kind = clang_getCursorKind(cursor);
      CXCursor parent = clang_getCursorSemanticParent(cursor);

      if ((kind == CXCursor_VarDecl) &&
          (clang_getCursorKind(parent) == CXCursor_TranslationUnit)) {
        GlobalVarInfo info;
        info.file = get_file_location(cursor);
        info.line = get_line_number(cursor);
        info.name = get_cursor_spelling(cursor);
        info.type = get_type_spelling(cursor);
        info.isConst = is_const(cursor);
        info.initValue = get_init_value(cursor);

        if (!info.name.empty() && !info.type.empty()) {
          globals_.push_back(info);
        }
      }
    }
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

  bool is_const(CXCursor cursor) {
    CXType type = clang_getCursorType(cursor);
    return clang_isConstQualifiedType(type);
  }

  std::string get_init_value(CXCursor cursor) {
    std::string result;
    CXCursor init_cursor = clang_Cursor_getVarDeclInitializer(cursor);
    if (!clang_Cursor_isNull(init_cursor)) {
      CXSourceRange range = clang_getCursorExtent(init_cursor);
      CXSourceLocation start = clang_getRangeStart(range);
      CXSourceLocation end = clang_getRangeEnd(range);

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
    }
    return result;
  }
};

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
      Document doc;
      doc.SetObject();
      auto& allocator = doc.GetAllocator();
      doc.AddMember("command", "find_global", allocator);
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

  GlobalVarFinder finder;
  for (const auto& file : cpp_files) {
    auto file_globals = finder.find_globals(file);
    globals.insert(globals.end(), file_globals.begin(), file_globals.end());
  }

  if (g_opts.output_json) {
    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    doc.AddMember("command", "find_global", allocator);
    doc.AddMember("path", StringRef(g_opts.path.c_str()), allocator);
    doc.AddMember("status", "success", allocator);
    doc.AddMember("count", static_cast<int>(globals.size()), allocator);

    Value globals_array(kArrayType);
    for (const auto& g : globals) {
      Value obj(kObjectType);
      obj.AddMember("name", StringRef(g.name.c_str()), allocator);
      obj.AddMember("type", StringRef(g.type.c_str()), allocator);
      obj.AddMember("file", StringRef(g.file.c_str()), allocator);
      obj.AddMember("line", g.line, allocator);
      obj.AddMember("isConst", g.isConst, allocator);
      if (!g.initValue.empty()) {
        obj.AddMember("initValue", StringRef(g.initValue.c_str()), allocator);
      }
      globals_array.PushBack(obj, allocator);
    }
    doc.AddMember("globals", globals_array, allocator);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::cout << buffer.GetString() << std::endl;
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