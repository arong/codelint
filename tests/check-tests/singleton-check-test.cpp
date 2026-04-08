#include "clang-tidy/ClangTidyTest.h"
#include "codelint/checks/SingletonCheck.h"
#include "gtest/gtest.h"

using namespace clang::tidy::test;
using namespace clang::tidy::codelint;

namespace {

TEST(SingletonCheckTest, MeyersSingleton) {
  std::string Code = R"(
class Singleton {
public:
  static Singleton& instance() {
    static Singleton inst;
    return inst;
  }
};
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<SingletonCheck>(Code, &Errors);

  EXPECT_EQ(1u, Errors.size());
  if (!Errors.empty()) {
    EXPECT_TRUE(Errors[0].Message.Message.find("Singleton") != std::string::npos);
    EXPECT_TRUE(Errors[0].Message.Message.find("instance") != std::string::npos);
  }
}

TEST(SingletonCheckTest, RegularFunctionNotReported) {
  std::string Code = R"(
int getValue() {
  int x = 42;
  return x;
}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<SingletonCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(SingletonCheckTest, StaticLocalNotReturned) {
  std::string Code = R"(
int getCounter() {
  static int counter = 0;
  counter++;
  return counter;
}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<SingletonCheck>(Code, &Errors);

  // This is NOT a singleton pattern - it returns int, not reference to static
  EXPECT_EQ(0u, Errors.size());
}

TEST(SingletonCheckTest, ReferenceToNonStatic) {
  std::string Code = R"(
int& getRef() {
  static int x = 42;
  return x;  // This IS a singleton pattern
}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<SingletonCheck>(Code, &Errors);

  // This is a singleton pattern
  EXPECT_EQ(1u, Errors.size());
}

TEST(SingletonCheckTest, PointerNotSingleton) {
  std::string Code = R"(
int* getPtr() {
  static int x = 42;
  return &x;
}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<SingletonCheck>(Code, &Errors);

  // Returns pointer, not reference - not a classic Meyers singleton
  EXPECT_EQ(0u, Errors.size());
}

} // anonymous namespace