// RUN: %codelint %s codelint-local-var-naming %t

void test() {
  int _var = 1;
  // CHECK-MESSAGES: :[@LINE-1]:7: warning: local variable '_var' should not start with '_'
  // [codelint-local-var-naming]

  int var_ = 2;
  // CHECK-MESSAGES: :[@LINE-1]:7: warning: local variable 'var_' should not end with '_'
  // [codelint-local-var-naming]

  int m_var = 3;
  // CHECK-MESSAGES: :[@LINE-1]:7: warning: local variable 'm_var' should not use 'm_' prefix
  // [codelint-local-var-naming]

  int valid = 4;  // OK - no underscore prefix/suffix
  int mvalid = 5; // OK - m not followed by underscore
  int var = 6;    // OK - no underscore at all
  int _ = 7;      // OK - standalone underscore is allowed (structured binding discard)
}
