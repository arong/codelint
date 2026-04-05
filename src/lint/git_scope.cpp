#include "lint/git_scope.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <regex>
#include <sstream>

namespace codelint {
namespace lint {

bool GitScope::run_git_command(const std::vector<std::string>& args, std::string& output) {
  std::ostringstream cmd;

  std::string repo_root;
  FILE* root_pipe = popen("git rev-parse --show-toplevel 2>/dev/null", "r");
  if (root_pipe) {
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), root_pipe)) {
      repo_root += buffer;
    }
    pclose(root_pipe);
    if (!repo_root.empty() && repo_root.back() == '\n') {
      repo_root.pop_back();
    }
    if (repo_root.find("not a git repository") != std::string::npos) {
      repo_root.clear();
    }
  }

  if (!repo_root.empty() && repo_root.find("fatal:") == std::string::npos) {
    cmd << "git -C \"" << repo_root << "\"";
  } else {
    cmd << "git";
  }

  for (const auto& arg : args) {
    cmd << " ";
    if (arg.find(' ') != std::string::npos || arg.find('"') != std::string::npos) {
      cmd << "\"" << arg << "\"";
    } else {
      cmd << arg;
    }
  }

  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) {
    error_ = "Failed to execute git command";
    return false;
  }

  char buffer[4096];
  output.clear();
  while (fgets(buffer, sizeof(buffer), pipe)) {
    output += buffer;
  }
  int status = pclose(pipe);

  if (status != 0) {
    error_ = "Git command failed with status: " + std::to_string(status);
    return false;
  }

  return status == 0 || output.empty();
}

LineRange GitScope::parse_hunk_header(const std::string& header) {
  LineRange range{0, 0};
  std::regex pattern(R"(@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@)");
  std::smatch match;

  if (std::regex_search(header, match, pattern)) {
    int start = std::stoi(match[1].str());
    int count = match[2].matched ? std::stoi(match[2].str()) : 1;

    range.start = start;
    range.end = start + count - 1;
    if (count == 0) {
      range.end = start;
    }
  }

  return range;
}

bool GitScope::parse_diff_output(const std::string& diff_output) {
  std::istringstream stream(diff_output);
  std::string line;
  std::string current_file;

  while (std::getline(stream, line)) {
    if (line.rfind("diff --git", 0) == 0) {
      size_t a_pos = line.find("a/");
      size_t b_pos = line.find("b/");
      if (a_pos != std::string::npos && b_pos != std::string::npos && b_pos > a_pos) {
        current_file = line.substr(b_pos + 2);
        if (!current_file.empty() && current_file.back() == '\r') {
          current_file.pop_back();
        }
        file_ranges_[current_file] = std::vector<LineRange>();
      }
    } else if (line.rfind("@@", 0) == 0 && !current_file.empty()) {
      LineRange range = parse_hunk_header(line);
      if (range.start > 0) {
        file_ranges_[current_file].push_back(range);
      }
    }
  }

  return true;
}

std::string GitScope::detect_main_branch() const {
  std::string output;

  std::ostringstream cmd;
  cmd << "git rev-parse --verify origin/main 2>/dev/null";
  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (pipe) {
    char buffer[256];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe)) {
      output += buffer;
    }
    pclose(pipe);
    if (output.find("fatal:") == std::string::npos) {
      return "origin/main";
    }
  }

  cmd.str("");
  cmd << "git rev-parse --verify origin/master 2>/dev/null";
  pipe = popen(cmd.str().c_str(), "r");
  if (pipe) {
    char buffer[256];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe)) {
      output += buffer;
    }
    pclose(pipe);
    if (output.find("fatal:") == std::string::npos) {
      return "origin/master";
    }
  }

  return "";
}

bool GitScope::init() {
    if (mode_ == Mode::ALL) {
        return true;
    }

    std::string diff_output;
    bool success = false;

    switch (mode_) {
        case Mode::MODIFIED:
            success = run_git_command({"diff", "-U0", "HEAD"}, diff_output);
            break;

        case Mode::STAGED:
            success = run_git_command({"diff", "--cached", "-U0"}, diff_output);
            break;

        case Mode::COMMIT:
            success = run_git_command({"diff", "-U0", ref_ + "^", ref_}, diff_output);
            break;

        case Mode::MERGE_BASE: {
            std::string main_branch = detect_main_branch();
            if (main_branch.empty()) {
                error_ = "Could not detect main branch (tried origin/main, origin/master)";
                return false;
            }
            success = run_git_command({"diff", "-U0", main_branch + "...HEAD"}, diff_output);
            break;
        }

  std::string diff_output;
  bool success = false;

  switch (mode_) {
  case Mode::MODIFIED:
    success = run_git_command({"diff", "-U0", "HEAD"}, diff_output);
    break;

  case Mode::COMMIT:
    success = run_git_command({"diff", "-U0", ref_ + "^", ref_}, diff_output);
    break;

  case Mode::MERGE_BASE: {
    std::string main_branch = detect_main_branch();
    if (main_branch.empty()) {
      error_ = "Could not detect main branch (tried origin/main, origin/master)";
      return false;
    }
    success = run_git_command({"diff", "-U0", main_branch + "...HEAD"}, diff_output);
    break;
  }

  case Mode::ALL:
    break;
  }

  if (!success) {
    return false;
  }

  return parse_diff_output(diff_output);
}

std::optional<GitScope> GitScope::parse(const std::string& scope_str) {
  GitScope scope;

  if (scope_str == "all") {
    scope.mode_ = Mode::ALL;
    return scope;
  }

  if (scope_str == "modified") {
    scope.mode_ = Mode::MODIFIED;
    if (!scope.init()) {
      return std::nullopt;
    }
    return scope;
  }

    if (scope_str == "staged") {
        scope.mode_ = Mode::STAGED;
        if (!scope.init()) {
            return std::nullopt;
        }
        return scope;
    }

    if (scope_str == "merge-base") {
        scope.mode_ = Mode::MERGE_BASE;
        if (!scope.init()) {
            return std::nullopt;
        }
        return scope;
    }
    return scope;
  }

  if (scope_str.rfind("commit:", 0) == 0) {
    scope.mode_ = Mode::COMMIT;
    scope.ref_ = scope_str.substr(7);
    if (scope.ref_.empty()) {
      return std::nullopt;
    }
    if (!scope.init()) {
      return std::nullopt;
    }
    return scope;
  }

  return std::nullopt;
}

std::vector<std::string> GitScope::get_modified_files() const {
  std::vector<std::string> files;
  for (const auto& [file, ranges] : file_ranges_) {
    files.push_back(file);
  }
  return files;
}

std::vector<LineRange> GitScope::get_line_ranges(const std::string& filepath) const {
  auto it = file_ranges_.find(filepath);
  if (it != file_ranges_.end()) {
    return it->second;
  }
  return {};
}

bool GitScope::is_line_modified(const std::string& filepath, int line) const {
  auto ranges = get_line_ranges(filepath);
  for (const auto& range : ranges) {
    if (line >= range.start && line <= range.end) {
      return true;
    }
  }
  return false;
}

} // namespace lint
} // namespace codelint
