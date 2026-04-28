// RUN: %check_codelint %s codelint-init %t -- -std=c++17
#include <cstddef>
#include <cstdint>

void test_auto_brace_to_equals() {
  int value = 10;
// CHECK-MESSAGES: :[@LINE]:7: warning: variable should use '{}' syntax for initialization  [codelint-init]

  // auto with direct brace init should use = assignment
  auto x{42};
// CHECK-MESSAGES: :[@LINE]:8: warning: auto type should use '=' assignment instead of brace initialization  [codelint-init]

  // auto* with brace init should use = assignment
  auto* p{&value};
// CHECK-MESSAGES: :[@LINE]:9: warning: auto type should use '=' assignment instead of brace initialization  [codelint-init]
  const auto* cp{&value};
// CHECK-MESSAGES: :[@LINE]:15: warning: auto type should use '=' assignment instead of brace initialization  [codelint-init]

  // auto& should not trigger (reference)
  auto& ref = value;

  // Correct: auto with = assignment (no change needed)
  auto correct = 42;
  auto* correct_ptr = &value;
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <cstddef>
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: void test_auto_brace_to_equals() {
// CHECK-FIXES:   int value{10};
// CHECK-FIXES:   auto x = 42;
// CHECK-FIXES:   auto* p = &value;
// CHECK-FIXES:   const auto* cp = &value;
// CHECK-FIXES:   auto& ref = value;
// CHECK-FIXES:   auto correct = 42;
// CHECK-FIXES:   auto* correct_ptr = &value;
// CHECK-FIXES: }
