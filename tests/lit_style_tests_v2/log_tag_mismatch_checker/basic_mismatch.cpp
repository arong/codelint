// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for basic log tag mismatch detection

// Mock log macro
#define LOG(msg) printf(msg)

void FuncA() {
  LOG("[FuncA] correct tag");
  // No warning expected - tag matches function

  LOG("[FuncB] wrong tag");
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: log tag 'FuncB' does not match enclosing function
  // 'FuncA'  [codelint-log-tag-mismatch]
}

void FuncB() {
  LOG("[FuncB] correct again");
}
