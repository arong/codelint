#include <CLI/CLI.hpp>
#include <iostream>

#include "commands.h"

GlobalOptions g_opts;

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

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  CLI11_PARSE(app, argc, argv);

  return 0;
}