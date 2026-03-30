#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace codelint { namespace lint {

// Represents a range of lines [start, end] (inclusive)
struct LineRange {
    int start;
    int end;
};

// GitScope parses git scope strings and provides information about modified files and lines.
//
// Supported scope formats:
// - "all" → All files in the repository
// - "modified" → Uncommitted changes (git diff HEAD)
// - "commit:HASH" → Changes in a specific commit (e.g., "commit:HEAD", "commit:abc123")
// - "merge-base" → Changes between merge-base and HEAD (git diff origin/main...HEAD)
class GitScope {
public:
    // Parse a scope string and return a GitScope object.
    // Returns nullopt if the scope string is invalid or git commands fail.
    static std::optional<GitScope> parse(const std::string& scope_str);

    // Get the list of modified files.
    // Returns empty vector if no changes or in "all" mode.
    std::vector<std::string> get_modified_files() const;

    // Get the line ranges for a specific file.
    // Returns empty vector if the file has no changes or is not tracked.
    std::vector<LineRange> get_line_ranges(const std::string& filepath) const;

    // Check if a specific line in a file was modified.
    // Returns true if the line falls within any modified range.
    bool is_line_modified(const std::string& filepath, int line) const;

    // Check if there was an error during initialization.
    bool has_error() const { return !error_.empty(); }

    // Get the error message if has_error() is true.
    const std::string& error() const { return error_; }

private:
    enum class Mode { ALL, MODIFIED, COMMIT, MERGE_BASE };

    Mode mode_;
    std::string ref_;
    std::string error_;
    std::map<std::string, std::vector<LineRange>> file_ranges_;

    bool run_git_command(const std::vector<std::string>& args, std::string& output);
    bool parse_diff_output(const std::string& diff_output);
    static LineRange parse_hunk_header(const std::string& header);
    std::string detect_main_branch() const;
    bool init();
};

}}
