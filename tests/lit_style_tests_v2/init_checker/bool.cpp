// RUN: %codelint %s codelint-init %t
void testb() {
  bool flag = false;
}

// === Expected Fixed Output ===
// CHECK-FIXES: void testb() {
// CHECK-FIXES:   bool flag{false};
// CHECK-FIXES: }
