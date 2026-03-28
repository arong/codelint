#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <map>

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>

#include "commands.h"

// Version info - defined in commands.h or CMake
#ifndef CODELINT_VERSION
#define CODELINT_VERSION "1.0.0"
#endif

GlobalOptions g_opts;
codelint::lint::LintConfig g_lint_config;

// LibTooling options category for lint subcommand
static llvm::cl::OptionCategory LintToolCategory("Lint Tool Options");

int main(int argc, char** argv) {
  // Init LLVM for LibTooling
  llvm::InitLLVM Ilvm(argc, argv, true);

  CLI::App app{"codelint - C++ code analysis tool"};

  // Global options via CLI11
  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  app.add_option("-p,--path", g_opts.path,
                 "Path to compile_commands.json directory");
  app.add_flag("--output-json", g_opts.output_json, "Output format as JSON");

  // Lint subcommand with CLI11 options
  CLI::App* lint_cmd = app.add_subcommand("lint", "Run lint checks on C++ code");
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
          std::map<std::string, codelint::lint::Severity>{
              {"error", codelint::lint::Severity::ERROR},
              {"warning", codelint::lint::Severity::WARNING},
              {"info", codelint::lint::Severity::INFO},
              {"hint", codelint::lint::Severity::HINT}},
          CLI::ignore_case));

  // Handle --version flag before CLI11 parsing
  if (show_version) {
    std::cout << "codelint version " << CODELINT_VERSION << "\n";
    std::cout << "Built with LLVM LibTooling\n";
    std::cout << "Target: C++14 (AUTOSAR 2014)" << std::endl;
    return 0;
  }

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  // Parse CLI11 options
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  // If lint subcommand was invoked, use CommonOptionsParser
  if (app.got_subcommand("lint")) {
    // Use the factory method to create CommonOptionsParser
    // This handles compilation database and source path parsing
    auto OptionsParserOrErr = clang::tooling::CommonOptionsParser::create(
        argc, const_cast<const char**>(argv), LintToolCategory,
        llvm::cl::OneOrMore, "codelint lint");

    if (!OptionsParserOrErr) {
      llvm::errs() << "Error parsing options: "
                   << OptionsParserOrErr.takeError() << "\n";
      return 1;
    }

    clang::tooling::CommonOptionsParser& OptionsParser = *OptionsParserOrErr;

    // OptionsParser.getCompilations() - compilation database
    // OptionsParser.getSourcePathList() - list of source files

    // Call the lint function (implementation will use OptionsParser)
    lint();
  }

  return 0;
}