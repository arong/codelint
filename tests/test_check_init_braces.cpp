#include <vector>

void test_brace_init() {
  std::vector<int> vec1 = {1, 2, 3};
  std::vector<int> vec2 = {4, 5, 6};

  std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}};
}

int main() {
  test_brace_init();
  return 0;
}