# codelint-signed-to-unsigned-return

Detects dangerous signed-to-unsigned conversions on function return values.

## Description

This check warns when a function that returns a signed integer type (like `ssize_t`
or `int`) is assigned to an unsigned variable. This pattern is dangerous because:

1. Many POSIX/system functions return `-1` to signal errors
2. When `-1` is implicitly cast to unsigned, it becomes a very large positive number
3. Error conditions become "success" values, leading to logic errors
4. Security vulnerabilities: buffers may be processed with invalid sizes

## Examples

### Example 1: Variable declaration

```cpp
// Before - DANGEROUS
size_t n = read(fd, buffer, count);
// If read() returns -1 (error), n becomes SIZE_MAX (huge value)

// After - CORRECT
ssize_t n = read(fd, buffer, count);
if (n < 0) {
    // Handle error properly
}
```

### Example 2: Assignment expression

```cpp
// Before - DANGEROUS
size_t n;
n = read(fd, buffer, count);

// After - CORRECT
ssize_t n;
n = read(fd, buffer, count);
if (n < 0) {
    // Handle error properly
}
```

### Example 3: open() return value

```cpp
// Before - DANGEROUS
unsigned fd = open("/path/to/file", O_RDONLY);
// If open() returns -1 (error), fd becomes UINT_MAX

// After - CORRECT
int fd = open("/path/to/file", O_RDONLY);
if (fd < 0) {
    // Handle error properly
}
```

## Checked Functions

The check applies to functions that return signed types commonly used for
error signaling:

| Category | Functions | Return Type |
|----------|-----------|-------------|
| **POSIX I/O** | `read`, `write` | `ssize_t` |
| **File operations** | `open`, `close`, `creat` | `int` |
| **File status** | `stat`, `fstat`, `lstat` | `int` |
| **Memory** | `mmap`, `munmap`, `mprotect` | `int` / `void*` |
| **Process** | `fork`, `wait`, `waitpid` | `pid_t` / `int` |
| **Network** | `socket`, `accept`, `connect` | `int` |
| **Network I/O** | `recv`, `send`, `recvfrom`, `sendto` | `ssize_t` |

## Why This Matters

### Security Vulnerabilities

```cpp
// DANGEROUS pattern
size_t bytes_read = read(fd, buffer, 1024);
process_buffer(buffer, bytes_read);  // If read() failed, bytes_read is SIZE_MAX!

// SAFE pattern
ssize_t bytes_read = read(fd, buffer, 1024);
if (bytes_read < 0) {
    handle_error();
    return;
}
process_buffer(buffer, bytes_read);
```

### Logic Errors

```cpp
// DANGEROUS: loop never terminates properly
size_t remaining = read(fd, buffer, count);
while (remaining > 0) {  // If read() returned -1, this is always true!
    // ...
}

// SAFE: proper error handling
ssize_t remaining = read(fd, buffer, count);
if (remaining < 0) {
    handle_error();
    return;
}
while (remaining > 0) {
    // ...
}
```

## Limitations

- Only checks direct assignments (not through intermediate variables)
- Does not detect signed-to-unsigned conversions in expressions
- Requires the function to be declared with a signed return type

## See Also

- [cert-err33-c](https://clang.llvm.org/extra/clang-tidy/checks/cert/err33-c.html) - Check for unused return values
- [bugprone-unused-return-value](https://clang.llvm.org/extra/clang-tidy/checks/bugprone/unused-return-value.html)
