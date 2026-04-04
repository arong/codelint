#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "lint/lint_types.h"

namespace codelint {
namespace lint {

class IssueReporter {
public:
  IssueReporter() = default;
  ~IssueReporter() = default;

  IssueReporter(const IssueReporter&) = delete;
  IssueReporter& operator=(const IssueReporter&) = delete;

  IssueReporter(IssueReporter&&) = default;
  IssueReporter& operator=(IssueReporter&&) = default;

  void add_issue(const LintIssue& issue);
  void add_issues(const std::vector<LintIssue>& issues);
  void print_json(std::ostream& output_stream = std::cout) const;
  void print_sarif(std::ostream& output_stream = std::cout) const;
  void print_text(std::ostream& output_stream = std::cout) const;

  int error_count() const {
    return error_count_;
  }
  int warning_count() const {
    return warning_count_;
  }
  int info_count() const {
    return info_count_;
  }
  int hint_count() const {
    return hint_count_;
  }
  int total_count() const {
    return error_count_ + warning_count_ + info_count_ + hint_count_;
  }

  const std::vector<LintIssue>& issues() const {
    return issues_;
  }
  void clear();

private:
  std::vector<LintIssue> issues_;
  int error_count_ = 0;
  int warning_count_ = 0;
  int info_count_ = 0;
  int hint_count_ = 0;
};

} // namespace lint
} // namespace codelint
