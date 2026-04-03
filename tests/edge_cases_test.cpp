#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

// Edge case tests for command utilities
class EdgeCasesTest : public ::testing::Test {
protected:
  std::string codelint_path;

  void SetUp() override {
    codelint_path = "/Users/aronic/Documents/codelint/build/codelint";
  }
};

// Test with empty directory
TEST_F(EdgeCasesTest, EmptyDirectory) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string empty_dir = "/tmp/empty_cpp_dir";
  std::filesystem::create_directory(empty_dir);

  std::string cmd = codelint_path + " find_global " + empty_dir;
  int result = system(cmd.c_str());

  std::filesystem::remove(empty_dir);
  EXPECT_EQ(result / 256, 1); // system() returns exit_code * 256
}

// Test with non-existent path
TEST_F(EdgeCasesTest, NonExistentPath) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " find_global /nonexistent/path/12345";
  int result = system(cmd.c_str());
  EXPECT_EQ(result / 256, 1); // system() returns exit_code * 256
}

// Test with nested directories
TEST_F(EdgeCasesTest, NestedDirectories) {
  if (!std::filesystem::exists(codelint_path)) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string nested = "/tmp/nested_cpp_test/level1/level2";
  std::filesystem::create_directories(nested);
  std::ofstream(nested + "/deep.cpp") << "int x;";

  std::string cmd = codelint_path + " find_global /tmp/nested_cpp_test";
  int result = system(cmd.c_str());

  std::filesystem::remove_all("/tmp/nested_cpp_test");
  EXPECT_TRUE(result == 0 || result == 1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
