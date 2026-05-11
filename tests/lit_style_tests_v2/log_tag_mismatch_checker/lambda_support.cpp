// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test that lambdas allow outer function name as tag

#define LOG(msg) printf(msg)

void OuterFunc() {
  auto lambda1 = []() {
    LOG("[OuterFunc] inside lambda"); // Should be valid - matches outer function
  };

  auto lambda2 = []() {
    LOG("[WrongFunc] wrong tag");
    // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'WrongFunc' does not match enclosing
    // function 'OuterFunc'  [codelint-log-tag-mismatch]
  };
}
