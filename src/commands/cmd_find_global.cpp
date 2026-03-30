#include "commands/cmd_find_global.h"

#include <filesystem>
#include <iostream>
#include <sstream>

#include "commands.h"
#include "lint/checkers/global_checker.h"
#include "lint/git_scope.h"
#include "lint/lint_types.h"

namespace {

std::vector<std::string> collect_cpp_files(const std::string& path) {
  std::vector<std::string> files;
  std::filesystem::path fs_path(path);

  try {
    if (std::filesystem::is_directory(fs_path)) {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(fs_path)) {
        if (entry.is_regular_file()) {
          std::string ext = entry.path().extension().string();
          if (ext == ".cpp" || ext == ".cc" || ext == ".c++" || ext == ".cxx" ||
              ext == ".h" || ext == ".hpp") {
            files.push_back(entry.path().string());
          }
        }
      }
    } else if (std::filesystem::is_regular_file(fs_path)) {
      files.push_back(fs_path.string());
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return files;
}

void print_issue_text(const codelint::lint::LintIssue& issue) {
  std::cout << issue.file << ":" << issue.line << ":" << issue.column << ": "
            << codelint::lint::severity_to_string(issue.severity) << ": "
            << issue.description << " [" << issue.checker_name << "]\n";
  std::cout << "  " << issue.name << " : " << issue.type_str << "\n";
}

void print_issue_json(const codelint::lint::LintIssue& issue) {
  std::cout << "    {\n";
  std::cout << "      \"type\": \"" << codelint::lint::check_type_to_string(issue.type) << "\",\n";
  std::cout << "      \"severity\": \"" << codelint::lint::severity_to_string(issue.severity) << "\",\n";
  std::cout << "      \"checker\": \"" << issue.checker_name << "\",\n";
  std::cout << "      \"name\": \"" << issue.name << "\",\n";
  std::cout << "      \"type_str\": \"" << issue.type_str << "\",\n";
  std::cout << "      \"file\": \"" << issue.file << "\",\n";
  std::cout << "      \"line\": " << issue.line << ",\n";
  std::cout << "      \"column\": " << issue.column << ",\n";
  std::cout << "      \"description\": \"" << issue.description << "\",\n";
  std::cout << "      \"suggestion\": \"" << issue.suggestion << "\",\n";
  std::cout << "      \"fixable\": " << (issue.fixable ? "true" : "false") << "\n";
  std::cout << "    }";
}

void format_output_json(const std::vector<codelint::lint::LintIssue>& issues) {
  std::cout << "{\n";
  std::cout << "  \"issues\": [\n";

  for (size_t i = 0; i < issues.size(); ++i) {
    print_issue_json(issues[i]);
    if (i < issues.size() - 1) {
      std::cout << ",";
    }
    std::cout << "\n";
  }

  std::cout << "  ]\n";
  std::cout << "}\n";
}

void format_output_text(const std::vector<codelint::lint::LintIssue>& issues) {
  for (const auto& issue : issues) {
    print_issue_text(issue);
  }

  int count = static_cast<int>(issues.size());
  if (count > 0) {
    std::cout << "Found " << count << " global variable(s)\n";
  } else {
    std::cout << "No global variables found\n";
  }
}

bool should_skip_file(const std::optional<codelint::lint::GitScope>& scope,
                      const std::string& filepath) {
  if (!scope.has_value()) {
    return false;
  }

  auto modified_files = scope->get_modified_files();
  if (modified_files.empty()) {
    return false;
  }

  for (const auto& mf : modified_files) {
    if (mf == filepath || filepath.find(mf) != std::string::npos) {
      return false;
    }
  }

  return true;
}

}  // namespace

int find_global(const GlobalOptions& opts, const FindGlobalOptions& global_opts) {
  using namespace codelint::lint;

  std::vector<std::string> files = collect_cpp_files(global_opts.path);

  if (files.empty()) {
    if (opts.output_json) {
      std::cout << "{\"issues\": []}\n";
    } else {
      std::cerr << "Error: No C++ files found in " << global_opts.path << std::endl;
    }
    return 1;
  }

  std::optional<GitScope> scope;
  if (!opts.scope.empty() && opts.scope != "all") {
    scope = GitScope::parse(opts.scope);
  }

  GlobalChecker checker(scope);
  std::vector<LintIssue> all_issues;

  for (const auto& filepath : files) {
    if (should_skip_file(scope, filepath)) {
      std::cout << "Skipping unmodified file: " << filepath << "\n";
      continue;
    }

    if (!opts.output_json) {
      std::cout << "Checking " << filepath << "...\n";
    }

    LintResult result = checker.check(filepath);
    all_issues.insert(all_issues.end(), result.issues.begin(), result.issues.end());
  }

  if (opts.output_json) {
    format_output_json(all_issues);
  } else {
    format_output_text(all_issues);
  }

  return 0;
}
