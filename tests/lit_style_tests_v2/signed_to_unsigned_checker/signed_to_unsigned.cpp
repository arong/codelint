// RUN: %check_codelint %s codelint-signed-to-unsigned-return %t -- -std=c++17
// Test file for codelint-signed-to-unsigned-return check
// Mock declarations for testing
typedef long ssize_t;
typedef unsigned long size_t;
typedef int mode_t;

ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
int open(const char* pathname, int flags, mode_t mode);
int close(int fd);

// NOLINT: buffer used for testing
char buffer[1024]{};

void test_var_decl_violations() {
  size_t n = read(0, buffer, 1024);
// CHECK-MESSAGES: :[@LINE]:3: warning: signed return value from 'read' (type 'ssize_t') is assigned to unsigned variable 'n' (type 'size_t'); this may cause errors when function returns negative values  [codelint-signed-to-unsigned-return]

  unsigned bytes = write(1, buffer, 5);
// CHECK-MESSAGES: :[@LINE]:3: warning: signed return value from 'write' (type 'ssize_t') is assigned to unsigned variable 'bytes' (type 'unsigned int'); this may cause errors when function returns negative values  [codelint-signed-to-unsigned-return]

  unsigned fd = open("/tmp/test", 0, 0);
// CHECK-MESSAGES: :[@LINE]:3: warning: signed return value from 'open' (type 'int') is assigned to unsigned variable 'fd' (type 'unsigned int'); this may cause errors when function returns negative values  [codelint-signed-to-unsigned-return]
  close(fd);
}

void test_var_decl_correct() {
  ssize_t n = read(0, buffer, 1024);
  if (n < 0) {
  }

  int fd = open("/tmp/test", 0, 0);
  if (fd < 0) {
  }
  close(fd);
}

void test_assignment_violations() {
  size_t n{};
  n = read(0, buffer, 1024);
// CHECK-MESSAGES: :[@LINE]:3: warning: signed return value from 'read' (type 'ssize_t') is assigned to unsigned expression (type 'size_t'); this may cause errors when function returns negative values  [codelint-signed-to-unsigned-return]

  unsigned fd{};
  fd = open("/tmp/test", 0, 0);
// CHECK-MESSAGES: :[@LINE]:3: warning: signed return value from 'open' (type 'int') is assigned to unsigned expression (type 'unsigned int'); this may cause errors when function returns negative values  [codelint-signed-to-unsigned-return]
  close(fd);
}

void test_assignment_correct() {
  ssize_t n{};
  n = read(0, buffer, 1024);
  if (n < 0) {
  }

  int fd{};
  fd = open("/tmp/test", 0, 0);
  if (fd < 0) {
  }
  close(fd);
}

// === Expected Fixed Output ===
// CHECK-FIXES: typedef long ssize_t;
// CHECK-FIXES: typedef unsigned long size_t;
// CHECK-FIXES: typedef int mode_t;
// CHECK-FIXES: ssize_t read(int fd, void* buf, size_t count);
// CHECK-FIXES: ssize_t write(int fd, const void* buf, size_t count);
// CHECK-FIXES: int open(const char* pathname, int flags, mode_t mode);
// CHECK-FIXES: int close(int fd);
// CHECK-FIXES: char buffer[1024]{};
// CHECK-FIXES: void test_var_decl_violations() {
// CHECK-FIXES:   size_t n = read(0, buffer, 1024);
// CHECK-FIXES:   unsigned bytes = write(1, buffer, 5);
// CHECK-FIXES:   unsigned fd = open("/tmp/test", 0, 0);
