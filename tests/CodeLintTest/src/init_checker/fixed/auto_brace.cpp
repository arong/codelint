#include <cstddef>
#include <cstdint>

void test_auto_brace_to_equals() {
  int value{10};

  // auto with direct brace init should use = assignment
  auto x = 42;

  // auto* with brace init should use = assignment
  auto* p = &value;
  const auto* cp = &value;

  // auto& should not trigger (reference)
  auto& ref = value;

  // Correct: auto with = assignment (no change needed)
  auto correct = 42;
  auto* correct_ptr = &value;
}
