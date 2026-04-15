// Test for enum type initialization
// Focus: enum, enum class

// 1. UNSCOPED ENUM
enum Color { RED = 0, GREEN, BLUE };
Color color1{};

// 2. SCOPED ENUM (enum class)
enum class ErrorCode {
  None = 1,
  Unknown = 2,
  Timeout = 3,
};
ErrorCode ec1;                     // will generate warning, but not fix
ErrorCode ec2{ErrorCode::None};    // will be fixed
ErrorCode ec3{ErrorCode::Unknown}; // OK

// 3. scoped enum has 0 value
enum class Status {
  OK = 0,
  Fail = 1,
};

Status sts{};              // shall be `{}` inited
Status sts1{Status::OK};   // shall be `{}` inited
Status sts2{Status::Fail}; // OK

void test_local_enum() {
  Color local_color{};
  ErrorCode local_ec; // will generate warning, but not fix
}
