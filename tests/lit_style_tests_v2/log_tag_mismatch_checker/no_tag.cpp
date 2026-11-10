// RUN: %check_codelint %s codelint-log-tag-mismatch %t
// Test that logs without tags don't trigger warnings

#define LOG(msg) printf(msg)

void FuncA() {
  LOG("no tag at all");
  LOG("just some message");
  LOG("brackets [but not identifier]");
  LOG("[123] numeric only");
}
