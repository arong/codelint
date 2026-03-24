#pragma once

#include <string>

struct GlobalOptions {
  bool output_json = false;
  std::string path = ".";
};

extern GlobalOptions g_opts;

void find_global();
void find_singleton();
void check_init();