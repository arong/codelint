#include <cstdint>

int global1;
unsigned global2;

int global3 = 1;
unsigned global4 = 2;
uint64_t global5 = 5;

int global6{};
unsigned int global7{};

int global8{1};
unsigned int global9{2U};
uint64_t global10{5UL};

void foo(int a, int b = 10) {
  // note: 对 lambda的 = 不做修改, 保持原样
  auto square = [](int x) { return x * x; };

  // note: 对 for 循环里面的 = 不做修改, 保持惯例
  for (int i = a; i < b; i++) {
    square(i);
  }

  // 对 auto 声明的变量, 不要修改 =
  auto answer = 42;
}
