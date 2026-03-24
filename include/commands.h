#pragma once

#include <string>

struct GlobalOptions {
  std::string path;
  bool output_json = false;
};

extern GlobalOptions g_opts;

void find_global();
void find_singleton();
void check_init();