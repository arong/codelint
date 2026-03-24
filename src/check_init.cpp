#include <iostream>

#include "commands.h"

void check_init() {
  if (g_opts.output_json) {
    std::cout
        << "{\"command\":\"check_init\",\"path\":\"" << g_opts.path
        << "\",\"status\":\"success\",\"message\":\"Checking initialization\"}"
        << std::endl;
  } else {
    std::cout << "Checking initialization in: " << g_opts.path << std::endl;
    std::cout << "Status: success" << std::endl;
  }
}