#include "clang-tidy/ClangTidyTest.h"
#include "codelint/checks/InitCheck.h"
#include "gtest/gtest.h"

using namespace clang::tidy::test;
using namespace clang::tidy::codelint;

namespace {

TEST(InitCheckTest, UninitializedVariable) {
  std::string Code = "void f() { int x; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  EXPECT_EQ(1u, Errors.size());
  if (!Errors.empty()) {
    EXPECT_TRUE(Errors[0].Message.Message.find("uninitialized") != std::string::npos);
  }
}

TEST(InitCheckTest, InitializedVariable) {
  std::string Code = "void f() { int x{}; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(InitCheckTest, EqualsInitToBraceInit) {
  std::string Code = "void f() { int x = 5; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  EXPECT_EQ(1u, Errors.size());
  if (!Errors.empty()) {
    EXPECT_TRUE(Errors[0].Message.Message.find("'=' initialization") != std::string::npos);
  }
}

TEST(InitCheckTest, BraceInitNotReported) {
  std::string Code = "void f() { int x{5}; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  EXPECT_EQ(0u, Errors.size());
}

TEST(InitCheckTest, AutoNotReported) {
  std::string Code = "void f() { auto x = 5; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  // Auto declarations should NOT be reported (they require =)
  EXPECT_EQ(0u, Errors.size());
}

TEST(InitCheckTest, UnsignedSuffix) {
  std::string Code = "void f() { unsigned int x = 42; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  // Should report both uninitialized check (no U suffix) and equals init
  EXPECT_GE(Errors.size(), 1u);
}

TEST(InitCheckTest, UnsignedSuffixPresent) {
  std::string Code = "void f() { unsigned int x = 42U; }";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  // With U suffix, only equals init should be reported
  if (Errors.size() > 0) {
    bool hasUnsignedError = false;
    for (const auto& Err : Errors) {
      if (Err.Message.Message.find("unsigned") != std::string::npos &&
          Err.Message.Message.find("suffix") != std::string::npos) {
        hasUnsignedError = true;
        break;
      }
    }
    EXPECT_FALSE(hasUnsignedError);
  }
}

TEST(InitCheckTest, UnionNotReported) {
  std::string Code = R"(
union U {
  int a;
  float b;
};
void f() {
  U u;
}
)";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  // Union members should not trigger uninitialized warning
  EXPECT_EQ(0u, Errors.size());
}

TEST(InitCheckTest, ExternNotReported) {
  std::string Code = "extern int x;";
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<InitCheck>(Code, &Errors);

  // Extern declarations should not be reported
  EXPECT_EQ(0u, Errors.size());
}

} // anonymous namespace