// RUN: %codelint %s codelint-init %t
#include <cstddef>
#include <cstdint>

int global1;
// CHECK-MESSAGES: :5:5: error: variable is not initialized  [codelint-init]
unsigned global2;
// CHECK-MESSAGES: :7:10: error: variable is not initialized  [codelint-init]

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

  // 正确格式,无需告警和修复
  constexpr size_t answerOfUniverse{42UL};

  // 应该跳过类型收紧
  int d = 3.14;
  // CHECK-MESSAGES: :37:7: warning: narrowing conversion from floating to integer; cannot use '{}'
  // initialization  [codelint-init]
}

int Init() {
  return 0;
}

void test_bool_from_int() {
  bool ok = true;
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <cstddef>
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: int global1{};
// CHECK-FIXES: unsigned global2{};
// CHECK-FIXES: int global3{1};
// CHECK-FIXES: unsigned global4{2U};
// CHECK-FIXES: uint64_t global5{5UL};
// CHECK-FIXES: int global6{};
// CHECK-FIXES: unsigned int global7{};
// CHECK-FIXES: int global8{1};
// CHECK-FIXES: unsigned int global9{2U};
// CHECK-FIXES: uint64_t global10{5UL};
// CHECK-FIXES: void foo(int a, int b = 10) {
// CHECK-FIXES:   auto square = [](int x) { return x * x; };
