// RUN: %check_codelint %s codelint-lint-code %t

#define MAGIC 10u
#define MAGIC2 10

void test_macro_init_no_broken_fix() {
  // Macro-expanded initializers must not receive suffix/brace fix-its that
  // corrupt the spelling (e.g. MAGICU / MAGIC2U).
  unsigned c = MAGIC;
  unsigned d = MAGIC2;
  (void)c;
  (void)d;
}
