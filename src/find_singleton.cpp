#include <iostream>

#include "commands.h"

void find_singleton() {
  if (g_opts.output_json) {
    std::cout << "{\"command\":\"find_singleton\",\"path\":\"" << g_opts.path
              << "\",\"status\":\"success\",\"message\":\"Finding singleton "
                 "instances\"}"
              << std::endl;
  } else {
    std::cout << "Finding singleton instances in: " << g_opts.path << std::endl;
    std::cout << "Status: success" << std::endl;
  }
}