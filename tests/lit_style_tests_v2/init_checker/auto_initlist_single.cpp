// RUN: %check_codelint %s codelint-init %t
// Test for auto type with brace-initialized initializer_list constructor
// Bug: auto resp = std::vector<uint8_t>{0U} should NOT warn about
// initializer_list constructor ambiguity because auto types should be skipped

#include <cstdint>
#include <string>
#include <vector>

void test_auto_single_element_brace() {
  auto resp = std::vector<uint8_t>{0U};
  // No CHECK-MESSAGES - auto type should skip initializer_list single element check

  auto v = std::vector<int>{42};
  // No CHECK-MESSAGES - same pattern with different element type
}

void test_non_auto_still_warns() {
  std::vector<int> non_auto_vec;
  // CHECK-MESSAGES: :[[@LINE-1]]:20: warning: variable is not explicitly initialized
  // [codelint-init]
}

// === Expected Fixed Output ===
// CHECK-FIXES: #include <cstdint>
// CHECK-FIXES: #include <vector>
// CHECK-FIXES: #include <string>
// CHECK-FIXES: void test_auto_single_element_brace() {
// CHECK-FIXES:   auto resp = std::vector<uint8_t>{0U};
// CHECK-FIXES:   auto v = std::vector<int>{42};
// CHECK-FIXES: }
// CHECK-FIXES: void test_non_auto_still_warns() {
// CHECK-FIXES:   std::vector<int> non_auto_vec{};
// CHECK-FIXES: }
