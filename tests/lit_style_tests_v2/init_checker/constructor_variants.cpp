// RUN: %check_codelint %s codelint-init %t

struct Defaulted {
  int b{};
  Defaulted() = default;
};

struct Deleted {
  int c{};
  Deleted() = delete;
};

struct Delegating {
  int d{};
  Delegating() : d(0) {
  }
  Delegating(int) : Delegating() {
  }
};

struct OutOfLine {
  int a{};
  OutOfLine();
};

OutOfLine::OutOfLine() : a(0) {
}

struct UninitCtor {
  int x;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: field is not initialized {{.*}}[codelint-init]
  // CHECK-MESSAGES: :[[@LINE-2]]:7: warning: member variable 'x' is not initialized in constructor
  // {{.*}}[codelint-init]
  UninitCtor() {
  }
};

void touch() {
  (void)sizeof(Defaulted);
  (void)sizeof(Delegating);
  (void)sizeof(OutOfLine);
  (void)sizeof(UninitCtor);
}
