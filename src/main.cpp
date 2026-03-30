#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <optional>

#include "commands.h"
#include "lint/git_scope.h"

#ifndef CODELINT_VERSION
#define CODELINT_VERSION "1.0.0"
#endif

GlobalOptions g_opts;
codelint::lint::LintConfig g_lint_config;
std::optional<codelint::lint::GitScope> g_scope;

int main(int argc, char** argv) {
  CLI::App app{"codelint - C++ code analysis tool"};

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  app.add_option("-p,--path", g_opts.path,
                 "Path to compile_commands.json directory");
  app.add_flag("--output-json", g_opts.output_json, "Output format as JSON");

  // Scope option controls incremental analysis.
  // When scope != "all", only reports issues on modified lines (line-level filtering)
  // and only compiles/analyzes modified files (file-level filtering).
  app.add_option("--scope", g_opts.scope,
      "Filter analysis to only check modified code (default: all). "
      "Modes:\n"
      "  all                   - Check all files (full analysis) (default)\n"
      "  modified              - Check uncommitted changes (working directory)\n"
      "  staged                - Check staged changes (git add, not committed)\n"
      "  commit:<HASH>         - Check changes in a specific commit\n"
      "  merge-base            - Check diff vs merge-base (for PRs)\n"
      "  pr:<branch>           - Check PR diff vs specified branch\n"
      "  diff:<a>...<b>        - Check diff between two branches\n"
      "\n"
      "Examples:\n"
      "  codelint lint src/                # Check everything (default)\n"
      "  codelint lint src/ --scope modified   # Only modified files/lines\n"
      "  codelint lint src/ --scope staged     # Only git add changes\n"
      "  codelint lint src/ --scope pr:develop # PR to develop branch\n"
      "  codelint lint src/ --scope diff:main...feature  # Diff between branches")
      ->default_val("all");

  CLI::App* lint_cmd = app.add_subcommand("lint", "Run lint checks on C++ code");
  lint_cmd->add_option("files", g_lint_config.files,
                       "Source files to check")
      ->expected(0, std::numeric_limits<size_t>::max());
  lint_cmd->add_option("--only", g_lint_config.only_checkers,
                       "Only run specific checkers (comma-separated)");
  lint_cmd->add_option("--exclude", g_lint_config.exclude_patterns,
                       "Exclude checkers (comma-separated)");
  lint_cmd->add_flag("--fix", g_lint_config.auto_fix,
                     "Automatically apply fixes where possible");
  lint_cmd->add_flag("--inplace", g_lint_config.inplace,
                     "Modify files in-place (requires --fix)");
  lint_cmd->add_option("--severity", g_lint_config.min_severity,
                       "Minimum severity to report (error, warning, info, hint)")
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, codelint::lint::Severity>{
              {"error", codelint::lint::Severity::ERROR},
              {"warning", codelint::lint::Severity::WARNING},
              {"info", codelint::lint::Severity::INFO},
              {"hint", codelint::lint::Severity::HINT}},
          CLI::ignore_case));

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  if (show_version) {
    std::cout << "codelint version " << CODELINT_VERSION << "\n";
    std::cout << "Built with LLVM LibTooling\n";
    std::cout << "Target: C++14 (AUTOSAR 2014)" << std::endl;
    return 0;
  }

  if (!g_opts.scope.empty() && g_opts.scope != "all") {
    g_scope = codelint::lint::GitScope::parse(g_opts.scope);
    if (!g_scope.has_value()) {
      std::cerr << "Error: Invalid scope '" << g_opts.scope << "'" << std::endl;
      return 1;
    }
    if (g_scope->has_error()) {
      std::cerr << "Error: " << g_scope->error() << std::endl;
      return 1;
    }
  }

  if (app.got_subcommand("lint")) {
    lint();
  }

  return 0;
}
