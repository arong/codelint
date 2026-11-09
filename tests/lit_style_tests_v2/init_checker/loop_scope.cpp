// RUN: %check_codelint %s codelint-init %t

void test_loop_body_uninit() {
  for (int i = 0; i < 1; ++i) {
    int inner;
    // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: variable is not initialized
    // [codelint-init]
    (void)i;
    (void)inner;
  }

  int arr[] = {1};
  for (int v : arr) {
    int inner2;
    // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: variable is not initialized
    // [codelint-init]
    (void)v;
    (void)inner2;
  }
}
