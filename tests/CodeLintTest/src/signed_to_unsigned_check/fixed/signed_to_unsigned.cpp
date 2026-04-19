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
  size_t n = read(0, buffer, 1024); // WARNING: signed return value from 'read' (type 'ssize_t') is
                                    // assigned to unsigned variable 'n' (type 'size_t')

  unsigned bytes =
      write(1, buffer, 5); // WARNING: signed return value from 'write' (type 'ssize_t') is assigned
                           // to unsigned variable 'bytes' (type 'unsigned int')

  unsigned fd = open("/tmp/test", 0, 0); // WARNING: signed return value from 'open' (type 'int') is
                                         // assigned to unsigned variable 'fd' (type 'unsigned int')
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
  size_t n;
  n = read(0, buffer, 1024); // WARNING: signed return value from 'read' (type 'ssize_t') is
                             // assigned to unsigned expression (type 'size_t')

  unsigned fd;
  fd = open("/tmp/test", 0, 0); // WARNING: signed return value from 'open' (type 'int') is assigned
                                // to unsigned expression (type 'unsigned int')
  close(fd);
}

void test_assignment_correct() {
  ssize_t n;
  n = read(0, buffer, 1024);
  if (n < 0) {
  }

  int fd;
  fd = open("/tmp/test", 0, 0);
  if (fd < 0) {
  }
  close(fd);
}
