#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <vector>

#include "commands.h"
#include "lint/git_scope.h"

#ifndef CODELINT_VERSION
#define CODELINT_VERSION "1.0.0"
#endif

GlobalOptions g_opts;
std::optional<codelint::lint::GitScope> g_scope;

int main(int argc, char** argv) {
  CLI::App app{"codelint - C++ code analysis tool"};

  // Global options
  app.add_flag("--version", g_opts.show_version, "Show version information with LLVM details");
  app.add_option("-p,--path", g_opts.path, "Path to compile_commands.json directory");
  app.add_flag("--output-json", g_opts.output_json,
              "Output issues in JSON format (DEPRECATED: use --output-sarif)")
      ->description("Output in JSON format for CI/CD integration (deprecated, use --output-sarif for SARIF format)");
  app.add_flag("--output-sarif", g_opts.output_sarif, "Output in SARIF format")
      ->description("Output issues in SARIF v2.1.0 format (machine-readable, GitHub Code Scanning compatible)");
  app.add_option("--scope", g_opts.scope,
                 "Control incremental analysis (default: all)\n"
                 "Examples:\n"
                 "  --scope modified      # Only modified files/lines\n"
                 "  --scope staged        # Only git-add changes\n"
                 "  --scope pr:develop    # PR difference vs develop")
      ->default_val("all");

  // Define subcommand options structs locally
  CheckInitOptions check_init_opts;
  FindGlobalOptions find_global_opts;
  FindSingletonOptions find_singleton_opts;

  // check_init subcommand - with fix and suppress options
  CLI::App* check_init_cmd =
      app.add_subcommand("check_init", "Check variable initialization styles");
  check_init_cmd->add_option("files", check_init_opts.files, "Source files to analyze")
      ->expected(0, static_cast<int>(std::numeric_limits<size_t>::max()))
      ->description("C++ source files or directories to analyze");
  check_init_cmd->add_flag("--fix", check_init_opts.fix, "Apply automatic fixes where possible");
  check_init_cmd->add_flag("--inplace", check_init_opts.inplace,
                           "Modify files in-place (use with --fix)");
  check_init_cmd->add_flag("--suppress-constant", check_init_opts.suppress_constant,
                           "Skip const/constexpr suggestions (for regression tests)");

  // find_global subcommand - detection only, no fix options
  CLI::App* find_global_cmd =
      app.add_subcommand("find_global", "Find global variables in codebase");
  find_global_cmd->add_option("path", find_global_opts.path, "Source file or directory to scan")
      ->required()
      ->description("File or directory path containing code to analyze for global variables");

  // find_singleton subcommand - detection only, no fix options
  CLI::App* find_singleton_cmd =
      app.add_subcommand("find_singleton", "Find singleton patterns in codebase");
  find_singleton_cmd
      ->add_option("path", find_singleton_opts.path, "Source file or directory to scan")
      ->required()
      ->description("File or directory path containing code to analyze for singleton patterns");

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  // Handle version flag
  if (g_opts.show_version) {
    std::cout << "codelint version " << CODELINT_VERSION << "\n";
#ifdef LLVM_VERSION_STRING
    std::cout << "Built with LLVM " << LLVM_VERSION_STRING << "\n";
#else
    std::cout << "Built with LLVM LibTooling\n";
#endif
    std::cout << "Target: C++14 (AUTOSAR 2014)" << std::endl;
    return 0;
  }

  if (g_opts.output_json && !g_opts.output_sarif) {
    std::cerr << "Warning: --output-json is deprecated. Use --output-sarif for SARIF v2.1.0 format."
              << std::endl;
  }

  // Initialize GitScope if scope is set
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

  // Execute appropriate subcommand
  if (app.got_subcommand(check_init_cmd)) {
    return check_init(g_opts, check_init_opts);
  } else if (app.got_subcommand(find_global_cmd)) {
    return find_global(g_opts, find_global_opts);
  } else if (app.got_subcommand(find_singleton_cmd)) {
    return find_singleton(g_opts, find_singleton_opts);
  }

  return 0;
}
