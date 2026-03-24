// Test for enum type initialization
// Focus: enum, enum class

// 1. UNSCOPED ENUM
enum Color { RED = 0, GREEN, BLUE };
Color color1;

// 2. SCOPED ENUM (enum class)
enum class ErrorCode {
  None = 1,
  Unknown = 2,
  Timeout = 3,
};
ErrorCode ec1;

void test_local_enum() {
  Color local_color;
  ErrorCode local_ec;
}
