#include <iostream>

// Template globals test
template<typename T>
T global_template_val = T{};

template<>
int global_template_val<int> = 3;

int use_global_template = global_template_val<int>;