// RUN: %codelint %s codelint-init %t
// Test for enum type initialization
// Focus: enum, enum class

// 1. UNSCOPED ENUM
enum Color { RED = 0, GREEN, BLUE };
Color color1;
// CHECK-MESSAGES: :7:7: error: variable is not initialized  [codelint-init]

// 2. SCOPED ENUM (enum class)
enum class ErrorCode {
  None = 1,
  Unknown = 2,
  Timeout = 3,
};
ErrorCode ec1;                     // will generate warning, but not fix
ErrorCode ec2 = ErrorCode::None;   // will be fixed
// CHECK-MESSAGES: :17:11: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
ErrorCode ec3{ErrorCode::Unknown}; // OK

// 3. scoped enum has 0 value
enum class Status {
  OK = 0,
  Fail = 1,
};

Status sts;                // shall be `{}` inited
// CHECK-MESSAGES: :27:8: error: variable is not initialized  [codelint-init]
Status sts1 = Status::OK;  // shall be `{}` inited
// CHECK-MESSAGES: :29:8: warning: variable should use '{}' syntax for initialization  [codelint-lint-code]
Status sts2{Status::Fail}; // OK

void test_local_enum() {
  Color local_color;
// CHECK-MESSAGES: :34:9: error: variable is not initialized  [codelint-init]
  ErrorCode local_ec; // will generate warning, but not fix
}

// === Expected Fixed Output ===
// CHECK-FIXES: enum Color { RED = 0, GREEN, BLUE };
// CHECK-FIXES: Color color1{};
// CHECK-FIXES: enum class ErrorCode {
// CHECK-FIXES:   None = 1,
// CHECK-FIXES:   Unknown = 2,
// CHECK-FIXES:   Timeout = 3,
// CHECK-FIXES: };
// CHECK-FIXES: ErrorCode ec1;                     // will generate warning, but not fix
// CHECK-FIXES: ErrorCode ec2{ErrorCode::None};    // will be fixed
// CHECK-FIXES: ErrorCode ec3{ErrorCode::Unknown}; // OK
// CHECK-FIXES: enum class Status {
// CHECK-FIXES:   OK = 0,
