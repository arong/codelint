// RUN: %check_codelint %s codelint-init %t
// Test for enum type initialization
// Focus: enum, enum class

// 1. UNSCOPED ENUM
enum Color { RED = 0, GREEN, BLUE };
Color color1;
// CHECK-MESSAGES: :[[@LINE-1]]:7: error: variable is not initialized  [codelint-init]

// 2. SCOPED ENUM (enum class)
enum class ErrorCode {
  None = 1,
  Unknown = 2,
  Timeout = 3,
};
ErrorCode ec1;                     // will generate msg, but not fix
ErrorCode ec2 = ErrorCode::None;   // will be fixed
ErrorCode ec3{ErrorCode::Unknown}; // OK

// 3. scoped enum has 0 value
enum class Status {
  OK = 0,
  Fail = 1,
};

Status sts; // shall be `{}` inited
// CHECK-MESSAGES: :[[@LINE-1]]:8: error: variable is not initialized  [codelint-init]
Status sts1 = Status::OK;  // shall be `{}` inited
Status sts2{Status::Fail}; // OK

void test_local_enum() {
  Color local_color;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: error: variable is not initialized  [codelint-init]
  ErrorCode local_ec; // will generate msg, but not fix
}
