// RUN: %codelint %s codelint-local-var-naming %t

void test() {
  int _var = 1; // VIOLATES: starts with underscore
  // CHECK-MESSAGES: :[@LINE-1]:8: warning: local variable '_var' should not start/end with '_' or
  // start with 'm_'  [codelint-local-var-naming]

  int var_ = 2; // VIOLATES: ends with underscore
  // CHECK-MESSAGES: :[@LINE-1]:8: warning: local variable 'var_' should not start/end with '_' or
  // start with 'm_'  [codelint-local-var-naming]

  int m_var = 3; // VIOLATES: starts with m_
  // CHECK-MESSAGES: :[@LINE-1]:8: warning: local variable 'm_var' should not start/end with '_' or
  // start with 'm_'  [codelint-local-var-naming]

  int valid = 4;  // OK - no underscore prefix/suffix
  int mvalid = 5; // OK - m not followed by underscore
  int var = 6;    // OK - no underscore at all
}
