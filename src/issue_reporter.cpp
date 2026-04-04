#include "lint/issue_reporter.h"

#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace codelint {
namespace lint {

// Helper: Read a specific line from a file
static std::string read_line_from_file(const std::string& filepath, int line_number) {
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

// Helper: Truncate long lines to 120 chars + "..."
static std::string truncate_line(const std::string& line, size_t max_len = 120) {
  if (line.length() > max_len) {
    return line.substr(0, max_len) + "...";
  }
  return line;
}

// Convert Severity enum to SARIF level string
// SARIF spec: https://docs.oasis-open.org/sarif/sarif/v2.1.0/sarif-v2.1.0.html#_Toc34317641
inline std::string severity_to_sarif_level(Severity severity) {
  switch (severity) {
  case Severity::ERROR:
    return "error";
  case Severity::WARNING:
    return "warning";
  case Severity::INFO:
    return "note";
  case Severity::HINT:
    return "none";
  default:
    return "note"; // Default to "note" for unknown severity
  }
}

void IssueReporter::add_issue(const LintIssue& issue) {
  issues_.push_back(issue);
  switch (issue.severity) {
  case Severity::ERROR:
    ++error_count_;
    break;
  case Severity::WARNING:
    ++warning_count_;
    break;
  case Severity::INFO:
    ++info_count_;
    break;
  case Severity::HINT:
    ++hint_count_;
    break;
  }
}

void IssueReporter::add_issues(const std::vector<LintIssue>& issues) {
  for (const auto& issue : issues) {
    add_issue(issue);
  }
}

void IssueReporter::print_json(std::ostream& output_stream) const {
  using namespace rapidjson;

  Document doc;
  doc.SetObject();

  Value issues_array(kArrayType);

  for (const auto& issue : issues_) {
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
  summary.AddMember("errors", error_count_, doc.GetAllocator());
  summary.AddMember("warnings", warning_count_, doc.GetAllocator());
  summary.AddMember("info", info_count_, doc.GetAllocator());
  summary.AddMember("hints", hint_count_, doc.GetAllocator());
  summary.AddMember("total", total_count(), doc.GetAllocator());

  doc.AddMember("summary", summary, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);

  output_stream << buffer.GetString() << std::endl;
}

void IssueReporter::print_sarif(std::ostream& output_stream) const {
  using namespace rapidjson;

  Document doc;
  doc.SetObject();

  Value schema_val;
  schema_val.SetString(
      "https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json",
      doc.GetAllocator());
  doc.AddMember("$schema", schema_val, doc.GetAllocator());

  Value version_val;
  version_val.SetString("2.1.0", doc.GetAllocator());
  doc.AddMember("version", version_val, doc.GetAllocator());

  Value runs_array(kArrayType);
  Value run(kObjectType);

  Value tool(kObjectType);
  Value driver(kObjectType);

  Value name_val;
  name_val.SetString("Codelint", doc.GetAllocator());
  driver.AddMember("name", name_val, doc.GetAllocator());

  Value ver_val;
  ver_val.SetString("1.0.0", doc.GetAllocator());
  driver.AddMember("version", ver_val, doc.GetAllocator());

  Value info_uri_val;
  info_uri_val.SetString("https://github.com/username/codelint", doc.GetAllocator());
  driver.AddMember("informationUri", info_uri_val, doc.GetAllocator());

  tool.AddMember("driver", driver, doc.GetAllocator());
  run.AddMember("tool", tool, doc.GetAllocator());

  Value results_array(kArrayType);

  for (const auto& issue : issues_) {
    Value result(kObjectType);

    Value rule_id_val;
    rule_id_val.SetString(check_type_to_string(issue.type).c_str(), doc.GetAllocator());
    result.AddMember("ruleId", rule_id_val, doc.GetAllocator());

    Value level_val;
    level_val.SetString(severity_to_sarif_level(issue.severity).c_str(), doc.GetAllocator());
    result.AddMember("level", level_val, doc.GetAllocator());

    Value message(kObjectType);
    Value desc_val;
    desc_val.SetString(issue.description.c_str(), doc.GetAllocator());
    message.AddMember("text", desc_val, doc.GetAllocator());
    result.AddMember("message", message, doc.GetAllocator());

    Value locations_array(kArrayType);
    Value location(kObjectType);
    Value physical_location(kObjectType);
    Value artifact_location(kObjectType);

    Value uri_val;
    uri_val.SetString(issue.file.c_str(), doc.GetAllocator());
    artifact_location.AddMember("uri", uri_val, doc.GetAllocator());

    Value uri_base_id_val;
    uri_base_id_val.SetString("SRCROOT", doc.GetAllocator());
    artifact_location.AddMember("uriBaseId", uri_base_id_val, doc.GetAllocator());

    physical_location.AddMember("artifactLocation", artifact_location, doc.GetAllocator());

    Value region(kObjectType);
    region.AddMember("startLine", issue.line, doc.GetAllocator());
    region.AddMember("startColumn", issue.column, doc.GetAllocator());
    physical_location.AddMember("region", region, doc.GetAllocator());

    location.AddMember("physicalLocation", physical_location, doc.GetAllocator());
    locations_array.PushBack(location, doc.GetAllocator());
    result.AddMember("locations", locations_array, doc.GetAllocator());

    if (issue.fixable && !issue.suggestion.empty()) {
      Value fixes_array(kArrayType);
      Value fix(kObjectType);

      Value fix_desc(kObjectType);
      Value fix_desc_text_val;
      fix_desc_text_val.SetString("Replace with brace initialization", doc.GetAllocator());
      fix_desc.AddMember("text", fix_desc_text_val, doc.GetAllocator());
      fix.AddMember("description", fix_desc, doc.GetAllocator());

      Value artifact_changes_array(kArrayType);
      Value artifact_change(kObjectType);

      Value replacement_regions_array(kArrayType);
      Value replacement_region(kObjectType);
      replacement_region.AddMember("startLine", issue.line, doc.GetAllocator());
      replacement_region.AddMember("startColumn", issue.column, doc.GetAllocator());
      replacement_region.AddMember("endLine", issue.line, doc.GetAllocator());
      replacement_region.AddMember("endColumn", static_cast<int>(issue.column + issue.type_str.length() + issue.name.length() + 5),
                                   doc.GetAllocator());
      replacement_regions_array.PushBack(replacement_region, doc.GetAllocator());
      artifact_change.AddMember("replacementRegions", replacement_regions_array, doc.GetAllocator());

      Value replacement_val;
      replacement_val.SetString(issue.suggestion.c_str(), doc.GetAllocator());
      artifact_change.AddMember("replacement", replacement_val, doc.GetAllocator());

      artifact_changes_array.PushBack(artifact_change, doc.GetAllocator());
      fix.AddMember("artifactChanges", artifact_changes_array, doc.GetAllocator());

      fixes_array.PushBack(fix, doc.GetAllocator());
      result.AddMember("fixes", fixes_array, doc.GetAllocator());
    }

    results_array.PushBack(result, doc.GetAllocator());
  }

  run.AddMember("results", results_array, doc.GetAllocator());
  runs_array.PushBack(run, doc.GetAllocator());
  doc.AddMember("runs", runs_array, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);

  output_stream << buffer.GetString() << std::endl;
}

void IssueReporter::print_text(std::ostream& output_stream) const {
  if (issues_.empty()) {
    output_stream << "No issues found." << std::endl;
    return;
  }

  output_stream << "========================================" << std::endl;
  output_stream << "Lint Report" << std::endl;
  output_stream << "========================================" << std::endl;
  output_stream << "Errors: " << error_count_ << std::endl;
  output_stream << "Warnings: " << warning_count_ << std::endl;
  output_stream << "Info: " << info_count_ << std::endl;
  output_stream << "Hints: " << hint_count_ << std::endl;
  output_stream << "Total: " << total_count() << std::endl;
  output_stream << "========================================" << std::endl;
  output_stream << std::endl;

  std::map<std::string, std::vector<LintIssue>> issues_by_file;
  for (const auto& issue : issues_) {
    issues_by_file[issue.file].push_back(issue);
  }

  for (const auto& [file, file_issues] : issues_by_file) {
    output_stream << "=== " << file << " (" << file_issues.size() << " issues) ===" << std::endl;
    for (const auto& issue : file_issues) {
      output_stream << "[" << severity_to_string(issue.severity) << "] ";
      output_stream << issue.line << ":" << issue.column;
      output_stream << " - " << issue.checker_name << std::endl;
      output_stream << "  " << issue.description << std::endl;

      std::string source_line = read_line_from_file(issue.file, issue.line);
      if (!source_line.empty()) {
        output_stream << "  | " << issue.line << " | " << truncate_line(source_line) << std::endl;
      } else {
        output_stream << "  | (source unavailable)" << std::endl;
      }

      if (!issue.suggestion.empty()) {
        output_stream << "  Suggestion: " << issue.suggestion << std::endl;
      }
      output_stream << std::endl;
    }
  }
}

void IssueReporter::clear() {
  issues_.clear();
  error_count_ = 0;
  warning_count_ = 0;
  info_count_ = 0;
  hint_count_ = 0;
}

} // namespace lint
} // namespace codelint
