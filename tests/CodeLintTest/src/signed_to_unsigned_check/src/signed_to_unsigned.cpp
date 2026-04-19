// Test file for codelint-signed-to-unsigned-return check
// Mock declarations for testing
typedef long ssize_t;
typedef unsigned long size_t;
typedef int mode_t;

ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
int open(const char* pathname, int flags, mode_t mode);
int close(int fd);

char buffer[1024];

void test_var_decl_violations() {
  // Violation: ssize_t return value assigned to size_t
  size_t n = read(0, buffer, 1024);

  // Violation: ssize_t return value assigned to unsigned
  unsigned bytes = write(1, buffer, 5);

  // Violation: int return value assigned to unsigned
  unsigned fd = open("/tmp/test", 0, 0);
  close(fd);
}

void test_var_decl_correct() {
  // Correct: ssize_t type receives ssize_t return
  ssize_t n = read(0, buffer, 1024);
  if (n < 0) {
    // handle error
  }

  // Correct: int type receives int return
  int fd = open("/tmp/test", 0, 0);
  if (fd < 0) {
    // handle error
  }
  close(fd);
}

void test_assignment_violations() {
  size_t n;
  // Violation: ssize_t assigned to size_t
  n = read(0, buffer, 1024);

  unsigned fd;
  // Violation: int assigned to unsigned
  fd = open("/tmp/test", 0, 0);
  close(fd);
}

void test_assignment_correct() {
  ssize_t n;
  // Correct: matching types
  n = read(0, buffer, 1024);
  if (n < 0) {
    // handle error
  }

  int fd;
  // Correct: matching types
  fd = open("/tmp/test", 0, 0);
  if (fd < 0) {
    // handle error
  }
  close(fd);
}
