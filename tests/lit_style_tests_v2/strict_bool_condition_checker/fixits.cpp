// RUN: %check_codelint %s codelint-strict-bool-condition %t

int get();
int* getp();

void test_fixits() {
  int x = 1;
  int mask = 1;
  if (x) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  if (x & mask) {
    // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: condition must be bool type, but got 'int'
    // [codelint-strict-bool-condition]
  }
  int* p = nullptr;
  if (!p) {
    // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: condition must be bool type, but got 'int *'
    // [codelint-strict-bool-condition]
  }
  if (int* q = getp()) {
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: condition must be bool type, but got 'int *'
    // [codelint-strict-bool-condition]
    (void)q;
  }
  (void)get();
}

// === Expected Fixed Output ===
// Parenthesize bitwise conditions; do not rewrite !p or condition variables.
// CHECK-FIXES:   if ((x) != 0) {
// CHECK-FIXES:   if ((x & mask) != 0) {
// CHECK-FIXES:   if (!p) {
// CHECK-FIXES:   if (int* q = getp()) {
