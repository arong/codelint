// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test for various log macro naming patterns

#define INFO_LOG(msg) printf(msg)
#define debug_log(msg) printf(msg)
#define WARN_MSG(msg) printf(msg)

void FuncA() {
  INFO_LOG("[FuncB] wrong");
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: log tag 'FuncB'

  debug_log("[FuncB] wrong");
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: log tag 'FuncB'

  WARN_MSG("[FuncB] wrong");
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: log tag 'FuncB'
}
