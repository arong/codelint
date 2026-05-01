// Test for local variable naming convention violations

void test_local_var_naming() {
  int _var = 1;  // VIOLATES: starts with underscore
  int var_ = 2;  // VIOLATES: ends with underscore
  int m_var = 3; // VIOLATES: starts with m_

  int valid = 4;  // OK
  int mvalid = 5; // OK (m not followed by underscore)
}
