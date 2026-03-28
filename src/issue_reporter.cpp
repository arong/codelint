#include "lint/issue_reporter.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace codelint {
namespace lint {

void IssueReporter::add_issue(const LintIssue& issue) {
    issues_.push_back(issue);
    switch (issue.severity) {
        case Severity::ERROR: ++error_count_; break;
        case Severity::WARNING: ++warning_count_; break;
        case Severity::INFO: ++info_count_; break;
        case Severity::HINT: ++hint_count_; break;
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

    for (const auto& issue : issues_) {
        output_stream << "[" << severity_to_string(issue.severity) << "] ";
        output_stream << issue.file << ":" << issue.line << ":" << issue.column;
        output_stream << " - " << issue.checker_name << std::endl;
        output_stream << "  " << issue.description << std::endl;
        if (!issue.suggestion.empty()) {
            output_stream << "  Suggestion: " << issue.suggestion << std::endl;
        }
        output_stream << std::endl;
    }
}

void IssueReporter::clear() {
    issues_.clear();
    error_count_ = 0;
    warning_count_ = 0;
    info_count_ = 0;
    hint_count_ = 0;
}

}  // namespace lint
}  // namespace codelint