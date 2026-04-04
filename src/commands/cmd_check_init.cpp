#include "commands/cmd_check_init.h"

#include <chrono>
#include <map>

#include "commands/cmd_utils.h"
#include "lint/checkers/const_checker.h"
#include "lint/checkers/init_checker.h"
#include "lint/fix_applier.h"
#include "lint/git_scope.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <CLI/CLI.hpp>
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace {

bool is_cpp_file(const std::string& path) {
  return path.ends_with(".cpp") || path.ends_with(".cc") || path.ends_with(".cxx") ||
         path.ends_with(".c++") || path.ends_with(".h") || path.ends_with(".hpp") ||
         path.ends_with(".hh") || path.ends_with(".hxx");
}

void collect_files_recursive(const std::string& path, std::vector<std::string>& files) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return;
  }

  if (S_ISREG(st.st_mode)) {
    if (is_cpp_file(path)) {
      files.push_back(path);
    }
    return;
  }

  if (S_ISDIR(st.st_mode)) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
      return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name == "." || name == "..") {
        continue;
      }
      std::string full_path = path + "/" + name;
      collect_files_recursive(full_path, files);
    }
    closedir(dir);
  }
}

std::string read_file_content(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string read_line_from_file(const std::string& filepath, int line_number) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return "";
  }

  std::string line;
  int current_line = 1;
  while (std::getline(file, line)) {
    if (current_line == line_number) {
      return line;
    }
    ++current_line;
  }
  return "";
}

std::string truncate_line(const std::string& line, size_t max_len = 120) {
  if (line.length() > max_len) {
    return line.substr(0, max_len) + "...";
  }
  return line;
}

bool write_file_content(const std::string& filepath, const std::string& content) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  file << content;
  return true;
}

bool should_skip_file(const std::string& filepath,
                      const std::optional<codelint::lint::GitScope>& scope) {
  if (!scope.has_value()) {
    return false;
  }

  if (scope->get_modified_files().empty()) {
    return false;
  }

  auto modified_files = scope->get_modified_files();
  for (const auto& mf : modified_files) {
    if (filepath == mf || filepath.find(mf) != std::string::npos) {
      return false;
    }
  }

  return true;
}

} // namespace

namespace {

// ... existing helper functions ...

// Helper to count severity types
void count_severities(const std::vector<codelint::lint::LintIssue>& issues, int& error_count,
                      int& warning_count, int& info_count, int& hint_count) {
  error_count = warning_count = info_count = hint_count = 0;
  for (const auto& issue : issues) {
    switch (issue.severity) {
    case codelint::lint::Severity::ERROR:
      ++error_count;
      break;
    case codelint::lint::Severity::WARNING:
      ++warning_count;
      break;
    case codelint::lint::Severity::INFO:
      ++info_count;
      break;
    case codelint::lint::Severity::HINT:
      ++hint_count;
      break;
    }
  }
}

// Helper to count fixable issues
int count_fixable(const std::vector<codelint::lint::LintIssue>& issues) {
  int fixable_count = 0;
  for (const auto& issue : issues) {
    if (issue.fixable) {
      ++fixable_count;
    }
  }
  return fixable_count;
}

} // namespace

namespace codelint {

std::vector<std::string> collect_cpp_files(const std::vector<std::string>& paths) {
  std::vector<std::string> files;
  for (const auto& path : paths) {
    collect_files_recursive(path, files);
  }
  return files;
}

std::vector<codelint::lint::LintIssue>
collect_issues(const std::vector<std::string>& paths,
               const std::optional<codelint::lint::GitScope>& scope, bool suppress_constant) {

  auto files = collect_cpp_files(paths);
  if (files.empty()) {
    std::cerr << "Error: No C++ files found in provided paths\n";
    return {};
  }

  codelint::lint::InitChecker init_checker(scope);
  codelint::lint::ConstChecker const_checker(scope);
  std::vector<codelint::lint::LintIssue> all_issues;

  for (const auto& filepath : files) {
    if (should_skip_file(filepath, scope)) {
      std::cout << "Skipping unmodified file: " << filepath << "\n";
      continue;
    }
    auto init_result = init_checker.check(filepath);
    all_issues.insert(all_issues.end(), init_result.issues.begin(), init_result.issues.end());

    if (!suppress_constant) {
      auto const_result = const_checker.check(filepath);
      all_issues.insert(all_issues.end(), const_result.issues.begin(), const_result.issues.end());
    }
  }
  return all_issues;
}

} // namespace codelint

namespace {

std::map<std::string, std::vector<codelint::lint::LintIssue>>
group_issues_by_file(const std::vector<codelint::lint::LintIssue>& issues) {
  std::map<std::string, std::vector<codelint::lint::LintIssue>> issues_by_file;
  for (const auto& issue : issues) {
    if (issue.fixable) {
      issues_by_file[issue.file].push_back(issue);
    }
  }
  return issues_by_file;
}

} // namespace

namespace codelint {

int apply_fixes(const std::vector<codelint::lint::LintIssue>& issues, bool inplace) {
  if (issues.empty()) {
    return 0;
  }

  auto issues_by_file = group_issues_by_file(issues);
  codelint::lint::FixApplier fix_applier;

  for (const auto& [filepath, file_issues] : issues_by_file) {
    std::string original_content = read_file_content(filepath);
    if (original_content.empty()) {
      std::cerr << "Warning: Cannot read file " << filepath << "\n";
      continue;
    }
    std::string modified_content = original_content;
    if (fix_applier.applyFixes(file_issues, original_content, modified_content, filepath)) {
      if (inplace) {
        if (write_file_content(filepath, modified_content)) {
          std::cout << "Applied " << fix_applier.getAppliedFixCount() << " fix(es) to " << filepath
                    << "\n";
        } else {
          std::cerr << "Error: Cannot write to file " << filepath << "\n";
        }
      } else {
        std::cout << modified_content;
      }
    }
  }
  return fix_applier.getAppliedFixCount();
}

} // namespace codelint

namespace {

using namespace codelint::lint;

void format_json_output(const std::vector<LintIssue>& issues, int fixable_count) {
  using namespace rapidjson;

  Document doc;
  doc.SetObject();

  Value issues_array(kArrayType);
  for (const auto& issue : issues) {
    Value issue_obj(kObjectType);

    Value type_val;
    type_val.SetString(check_type_to_string(issue.type).c_str(), doc.GetAllocator());
    issue_obj.AddMember("type", type_val, doc.GetAllocator());

    Value sev_val;
    sev_val.SetString(severity_to_string(issue.severity).c_str(), doc.GetAllocator());
    issue_obj.AddMember("severity", sev_val, doc.GetAllocator());

    Value checker_val;
    checker_val.SetString(issue.checker_name.c_str(), doc.GetAllocator());
    issue_obj.AddMember("checker", checker_val, doc.GetAllocator());

    Value name_val;
    name_val.SetString(issue.name.c_str(), doc.GetAllocator());
    issue_obj.AddMember("name", name_val, doc.GetAllocator());

    Value type_str_val;
    type_str_val.SetString(issue.type_str.c_str(), doc.GetAllocator());
    issue_obj.AddMember("type_str", type_str_val, doc.GetAllocator());

    Value file_val;
    file_val.SetString(issue.file.c_str(), doc.GetAllocator());
    issue_obj.AddMember("file", file_val, doc.GetAllocator());

    issue_obj.AddMember("line", issue.line, doc.GetAllocator());
    issue_obj.AddMember("column", issue.column, doc.GetAllocator());

    Value desc_val;
    desc_val.SetString(issue.description.c_str(), doc.GetAllocator());
    issue_obj.AddMember("description", desc_val, doc.GetAllocator());

    Value sugg_val;
    sugg_val.SetString(issue.suggestion.c_str(), doc.GetAllocator());
    issue_obj.AddMember("suggestion", sugg_val, doc.GetAllocator());

    issue_obj.AddMember("fixable", issue.fixable, doc.GetAllocator());

    issues_array.PushBack(issue_obj, doc.GetAllocator());
  }

  doc.AddMember("issues", issues_array, doc.GetAllocator());

  Value summary(kObjectType);
  int error_count = 0, warning_count = 0, info_count = 0, hint_count = 0;
  for (const auto& issue : issues) {
    switch (issue.severity) {
    case Severity::ERROR:
      ++error_count;
      break;
    case Severity::WARNING:
      ++warning_count;
      break;
    case Severity::INFO:
      ++info_count;
      break;
    case Severity::HINT:
      ++hint_count;
      break;
    }
  }
  summary.AddMember("errors", error_count, doc.GetAllocator());
  summary.AddMember("warnings", warning_count, doc.GetAllocator());
  summary.AddMember("info", info_count, doc.GetAllocator());
  summary.AddMember("hints", hint_count, doc.GetAllocator());
  summary.AddMember("total", static_cast<int>(issues.size()), doc.GetAllocator());
  summary.AddMember("fixable", fixable_count, doc.GetAllocator());

  doc.AddMember("summary", summary, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);

  std::cout << buffer.GetString() << std::endl;
}

void format_text_output(const std::vector<LintIssue>& issues, int fixable_count) {
  if (issues.empty()) {
    std::cout << "No issues found.\n";
    return;
  }

  std::map<std::string, std::vector<LintIssue>> issues_by_file;
  for (const auto& issue : issues) {
    issues_by_file[issue.file].push_back(issue);
  }

  for (const auto& [file, file_issues] : issues_by_file) {
    std::cout << "=== " << file << " (" << file_issues.size() << " issues) ===\n";
    for (const auto& issue : file_issues) {
      std::cout << issue.line << ":" << issue.column << ": "
                << severity_to_string(issue.severity) << ": " << issue.description << " ["
                << issue.checker_name << "]\n";

      std::string source_line = read_line_from_file(issue.file, issue.line);
      if (!source_line.empty()) {
        std::cout << "  | " << issue.line << " | " << truncate_line(source_line) << "\n";
      } else {
        std::cout << "  | (source unavailable)\n";
      }

      if (!issue.suggestion.empty()) {
        std::cout << "  suggestion: " << issue.suggestion << "\n";
      }
    }
  }

  std::cout << "\n";
  int error_count = 0, warning_count = 0, info_count = 0, hint_count = 0;
  for (const auto& issue : issues) {
    switch (issue.severity) {
    case Severity::ERROR:
      ++error_count;
      break;
    case Severity::WARNING:
      ++warning_count;
      break;
    case Severity::INFO:
      ++info_count;
      break;
    case Severity::HINT:
      ++hint_count;
      break;
    }
  }

  if (error_count > 0) {
    std::cout << error_count << " error(s)";
  }
  if (warning_count > 0) {
    if (error_count > 0)
      std::cout << ", ";
    std::cout << warning_count << " warning(s)";
  }
  if (info_count > 0) {
    if (error_count + warning_count > 0)
      std::cout << ", ";
    std::cout << info_count << " info(s)";
  }
  if (hint_count > 0) {
    if (error_count + warning_count + info_count > 0)
      std::cout << ", ";
    std::cout << hint_count << " hint(s)";
  }
  if (fixable_count > 0) {
    std::cout << ", " << fixable_count << " fixable";
  }
  std::cout << "\n";
}

} // namespace

namespace codelint {

void format_sarif_output(const std::vector<LintIssue>& issues) {
  using namespace rapidjson;

  Document doc;
  doc.SetObject();

  Value schema;
  schema.SetString("$schema", doc.GetAllocator());
  doc.AddMember(schema, Value("https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json", doc.GetAllocator()).Move(), doc.GetAllocator());

  Value version;
  version.SetString("version", doc.GetAllocator());
  doc.AddMember(version, Value("2.1.0", doc.GetAllocator()).Move(), doc.GetAllocator());

  Value runs(kArrayType);

  Value run_obj(kObjectType);

  Value tool(kObjectType);
  Value driver(kObjectType);
  Value driver_name;
  driver_name.SetString("name", doc.GetAllocator());
  driver.AddMember(driver_name, Value("Codelint", doc.GetAllocator()).Move(), doc.GetAllocator());
  Value driver_version;
  driver_version.SetString("version", doc.GetAllocator());
  driver.AddMember(driver_version, Value("0.1.0", doc.GetAllocator()).Move(), doc.GetAllocator());
  tool.AddMember("driver", driver, doc.GetAllocator());
  run_obj.AddMember("tool", tool, doc.GetAllocator());

  Value results(kArrayType);
  for (const auto& issue : issues) {
    Value result(kObjectType);

    Value rule_id;
    rule_id.SetString("ruleId", doc.GetAllocator());
    result.AddMember(rule_id, Value(check_type_to_string(issue.type).c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());

    Value level;
    level.SetString("level", doc.GetAllocator());
    result.AddMember(level, Value(severity_to_string(issue.severity) == "error" ? "error" : "warning", doc.GetAllocator()).Move(), doc.GetAllocator());

    Value message(kObjectType);
    Value msg_text;
    msg_text.SetString("text", doc.GetAllocator());
    message.AddMember(msg_text, Value(issue.description.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
    result.AddMember("message", message, doc.GetAllocator());

    Value locations(kArrayType);
    Value location(kObjectType);
    Value phys_loc(kObjectType);
    Value art_loc(kObjectType);
    Value uri;
    uri.SetString("uri", doc.GetAllocator());
    std::string encoded_path = issue.file;
    for (size_t i = 0; i < encoded_path.length(); ++i) {
      char c = encoded_path[i];
      if (c == ' ') {
        encoded_path.replace(i, 1, "%20");
        i += 2;
      } else if (c == '"') {
        encoded_path.replace(i, 1, "%22");
        i += 2;
      }
    }
    art_loc.AddMember(uri, Value(encoded_path.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
    Value uri_base;
    uri_base.SetString("uriBaseId", doc.GetAllocator());
    art_loc.AddMember(uri_base, Value("SRCROOT", doc.GetAllocator()).Move(), doc.GetAllocator());
    phys_loc.AddMember("artifactLocation", art_loc, doc.GetAllocator());

    Value region(kObjectType);
    Value start_line;
    start_line.SetString("startLine", doc.GetAllocator());
    region.AddMember(start_line, issue.line, doc.GetAllocator());
    Value start_col;
    start_col.SetString("startColumn", doc.GetAllocator());
    region.AddMember(start_col, issue.column, doc.GetAllocator());
    phys_loc.AddMember("region", region, doc.GetAllocator());
    location.AddMember("physicalLocation", phys_loc, doc.GetAllocator());
    locations.PushBack(location, doc.GetAllocator());
    result.AddMember("locations", locations, doc.GetAllocator());

    if (issue.fixable) {
      Value fixes(kArrayType);
      Value fix(kObjectType);
      Value fix_desc(kObjectType);
      Value fix_desc_text;
      fix_desc_text.SetString("text", doc.GetAllocator());
      fix_desc.AddMember(fix_desc_text, Value("Replace with brace initialization", doc.GetAllocator()).Move(), doc.GetAllocator());
      fix.AddMember("description", fix_desc, doc.GetAllocator());

      Value art_changes(kArrayType);
      Value art_change(kObjectType);
      Value repl_regions(kArrayType);
      Value repl_region(kObjectType);
      Value r_start_line;
      r_start_line.SetString("startLine", doc.GetAllocator());
      repl_region.AddMember(r_start_line, issue.line, doc.GetAllocator());
      Value r_start_col;
      r_start_col.SetString("startColumn", doc.GetAllocator());
      repl_region.AddMember(r_start_col, issue.column, doc.GetAllocator());
      Value r_end_line;
      r_end_line.SetString("endLine", doc.GetAllocator());
      repl_region.AddMember(r_end_line, issue.line, doc.GetAllocator());
      Value r_end_col;
      r_end_col.SetString("endColumn", doc.GetAllocator());
      repl_region.AddMember(r_end_col, issue.column + (int)issue.name.size() + (issue.type_str.empty() ? 0 : (int)issue.type_str.size() + 1), doc.GetAllocator());
      repl_regions.PushBack(repl_region, doc.GetAllocator());
      Value replacement;
      replacement.SetString(issue.suggestion.c_str(), doc.GetAllocator());
      art_change.AddMember("replacementRegions", repl_regions, doc.GetAllocator());
      art_change.AddMember("replacement", replacement, doc.GetAllocator());
      art_changes.PushBack(art_change, doc.GetAllocator());
      fix.AddMember("artifactChanges", art_changes, doc.GetAllocator());
      fixes.PushBack(fix, doc.GetAllocator());
      result.AddMember("fixes", fixes, doc.GetAllocator());
    }

    results.PushBack(result, doc.GetAllocator());
  }

  run_obj.AddMember("results", results, doc.GetAllocator());
  runs.PushBack(run_obj, doc.GetAllocator());
  doc.AddMember("runs", runs, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  std::cout << buffer.GetString() << std::endl;
}

int format_output(const std::vector<LintIssue>& issues, bool output_json, bool output_sarif) {
  int fixable_count = count_fixable(issues);

  if (output_sarif) {
    format_sarif_output(issues);
  } else if (output_json) {
    format_json_output(issues, fixable_count);
  } else {
    format_text_output(issues, fixable_count);
  }

  return issues.size() > 0 ? 1 : 0;
}

} // namespace codelint

int check_init(const GlobalOptions& opts, const CheckInitOptions& init_opts) {
  using namespace codelint::lint;

  auto start_time = std::chrono::high_resolution_clock::now();

  std::optional<GitScope> scope;
  if (!opts.scope.empty() && opts.scope != "all") {
    scope = GitScope::parse(opts.scope);
  }

  auto files = codelint::collect_cpp_files(init_opts.files);
  int files_processed = static_cast<int>(files.size());

  auto all_issues = codelint::collect_issues(init_opts.files, scope, init_opts.suppress_constant);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  int error_count = 0, warning_count = 0, info_count = 0, hint_count = 0;
  count_severities(all_issues, error_count, warning_count, info_count, hint_count);

  int fixable_count = count_fixable(all_issues);

  if (all_issues.empty() && !init_opts.fix) {
    int result = codelint::format_output(all_issues, opts.output_json, opts.output_sarif);
    if (!opts.output_json && !opts.output_sarif) {
      print_statistics(files_processed, static_cast<int>(all_issues.size()), elapsed_ms,
                       error_count, warning_count, info_count, fixable_count);
    }
    return result;
  }

  if (init_opts.fix && fixable_count > 0) {
    codelint::apply_fixes(all_issues, init_opts.inplace);
    return 0;
  }

  int result = codelint::format_output(all_issues, opts.output_json, opts.output_sarif);
  if (!opts.output_json && !opts.output_sarif) {
    print_statistics(files_processed, static_cast<int>(all_issues.size()), elapsed_ms,
                     error_count, warning_count, info_count, fixable_count);
  }
  return result;
}
