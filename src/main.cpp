#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <map>

#include "commands.h"

GlobalOptions g_opts;
CheckInitOptions g_check_init_opts;
cndy::lint::LintConfig g_lint_config;

int main(int argc, char** argv) {
  CLI::App app{"cndy - C++ code analysis tool"};

  app.add_option("-p,--path", g_opts.path,
                 "Path to compile_commands.json directory")
      ->default_str(".");
  app.add_flag("--output-json", g_opts.output_json, "Output format as JSON");

  CLI::App* find_global_cmd =
      app.add_subcommand("find_global", "Find global variables");
  find_global_cmd->callback(find_global);

  CLI::App* find_singleton_cmd =
      app.add_subcommand("find_singleton", "Find singleton instances");
  find_singleton_cmd->callback(find_singleton);

  CLI::App* check_init_cmd =
      app.add_subcommand("check_init", "Check initialization");
  check_init_cmd->callback(check_init);
  check_init_cmd
      ->add_option("files", g_check_init_opts.files,
                   "Source files to check (if not specified, checks all files "
                   "from compile_commands.json)")
      ->expected(0, std::numeric_limits<size_t>::max());
  check_init_cmd->add_flag(
      "--fix", g_opts.fix,
      "Automatically fix initialization issues and output to stdout");
  check_init_cmd->add_flag("--inplace", g_opts.inplace,
                           "Modify files in-place (requires --fix)");

  CLI::App* lint_cmd = app.add_subcommand("lint", "Run lint checks on C++ code");
  lint_cmd->callback(lint);
  lint_cmd->add_option("files", g_lint_config.files,
                       "Source files to check")
      ->expected(0, std::numeric_limits<size_t>::max());
  lint_cmd->add_option("--only", g_lint_config.only_checkers,
                       "Only run specific checkers (comma-separated)")
      ->delimiter(',');
  lint_cmd->add_option("--exclude", g_lint_config.exclude_patterns,
                       "Exclude checkers (comma-separated)")
      ->delimiter(',');
  lint_cmd->add_flag("--fix", g_lint_config.auto_fix,
                     "Automatically apply fixes where possible");
  lint_cmd->add_flag("--inplace", g_lint_config.inplace,
                     "Modify files in-place (requires --fix)");
  lint_cmd->add_option("--severity", g_lint_config.min_severity,
                       "Minimum severity to report (error, warning, info, hint)")
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, cndy::lint::Severity>{
              {"error", cndy::lint::Severity::ERROR},
              {"warning", cndy::lint::Severity::WARNING},
              {"info", cndy::lint::Severity::INFO},
              {"hint", cndy::lint::Severity::HINT}},
          CLI::ignore_case));

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  CLI11_PARSE(app, argc, argv);

  return 0;
}