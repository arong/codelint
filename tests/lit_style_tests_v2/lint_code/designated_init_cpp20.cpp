// RUN: %check_codelint %s codelint-lint-code %t

struct Point {
  int x;
  int y;
};

struct Color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

struct Config {
  int width;
  int height;
  bool fullscreen;
};

void test_designated_init() {
  Point p1{1, 2};
  // CHECK-MESSAGES: :[[@LINE-1]]:11: warning: use designated initializers for aggregate
  // initialization [codelint-lint-code]

  Color c{255, 128, 0};
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: use designated initializers for aggregate
  // initialization [codelint-lint-code]

  Config cfg{1920, 1080, true};
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: use designated initializers for aggregate
  // initialization [codelint-lint-code]
}

void test_already_designated() {
  Point p2{.x = 1, .y = 2};
  Color c2{.r = 255, .g = 128, .b = 0};
  Config cfg2{.width = 1920, .height = 1080, .fullscreen = true};
}

void test_empty_init() {
  Point p3{};
}

// === Expected Fixed Output ===
// CHECK-FIXES: void test_designated_init() {
// CHECK-FIXES:   Point p1{.x=1, .y=2};
// CHECK-FIXES:   Color c{.r=255, .g=128, .b=0};
// CHECK-FIXES:   Config cfg{.width=1920, .height=1080, .fullscreen=true};
// CHECK-FIXES: }
// CHECK-FIXES: void test_already_designated() {
// CHECK-FIXES:   Point p2{.x = 1, .y = 2};
// CHECK-FIXES:   Color c2{.r = 255, .g = 128, .b = 0};
// CHECK-FIXES:   Config cfg2{.width = 1920, .height = 1080, .fullscreen = true};
// CHECK-FIXES: }
// CHECK-FIXES: void test_empty_init() {
// CHECK-FIXES:   Point p3{};
// CHECK-FIXES: }
