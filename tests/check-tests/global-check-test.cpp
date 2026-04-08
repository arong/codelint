#include "clang-tidy/ClangTidyTest.h"
#include "codelint/checks/GlobalCheck.h"
#include "gtest/gtest.h"

using namespace clang::tidy::test;
using namespace clang::tidy::codelint;

namespace {

TEST(GlobalCheckTest, GlobalVariable) {
  std::string Code = "int global_var = 42;";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  EXPECT_EQ(1u, Errors.size());
  if (!Errors.empty()) {
    EXPECT_TRUE(Errors[0].Message.Message.find("global variable") != std::string::npos);
    EXPECT_TRUE(Errors[0].Message.Message.find("global_var") != std::string::npos);
  }
}

TEST(GlobalCheckTest, LocalVariableNotReported) {
  std::string Code = "void f() { int local = 42; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(GlobalCheckTest, StaticLocalNotReported) {
  std::string Code = "void f() { static int local = 42; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  // Static local variables should not be reported as globals
  EXPECT_EQ(0u, Errors.size());
}

TEST(GlobalCheckTest, FunctionParameterNotReported) {
  std::string Code = "void f(int param) {}";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(GlobalCheckTest, ClassMemberNotReported) {
  std::string Code = R"(
class MyClass {
  int member_var;
};
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(GlobalCheckTest, ConstGlobal) {
  std::string Code = "const int kGlobal = 42;";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  // Const globals are still globals and should be reported
  EXPECT_EQ(1u, Errors.size());
}

TEST(GlobalCheckTest, MultipleGlobals) {
  std::string Code = R"(
int global1 = 1;
int global2 = 2;
void f() {}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<GlobalCheck>(Code, &Errors);

  EXPECT_EQ(2u, Errors.size());
}

} // anonymous namespace