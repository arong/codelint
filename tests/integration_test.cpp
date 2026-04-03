#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>

// Integration tests - test --fix, --inplace, --output-json option combinations
class IntegrationTest : public ::testing::Test {
protected:
  std::string codelint_path;

  void SetUp() override {
    codelint_path = "/Users/aronic/Documents/codelint/build/codelint";
  }

  bool codelintExists() {
    return std::filesystem::exists(codelint_path);
  }
};

// Test --help outputs proper content for check_init
TEST_F(IntegrationTest, CheckInitHelpOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " check_init --help";
  FILE* pipe = popen(cmd.c_str(), "r");
  ASSERT_NE(pipe, nullptr);

  char buffer[1024];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  EXPECT_TRUE(output.find("check_init") != std::string::npos);
}

// Test find_global --help
TEST_F(IntegrationTest, FindGlobalHelpOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " find_global --help";
  FILE* pipe = popen(cmd.c_str(), "r");
  ASSERT_NE(pipe, nullptr);

  char buffer[1024];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  EXPECT_TRUE(output.find("find_global") != std::string::npos);
}

// Test find_singleton --help
TEST_F(IntegrationTest, FindSingletonHelpOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " find_singleton --help";
  FILE* pipe = popen(cmd.c_str(), "r");
  ASSERT_NE(pipe, nullptr);

  char buffer[1024];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  EXPECT_TRUE(output.find("find_singleton") != std::string::npos);
}

// Test JSON output format
TEST_F(IntegrationTest, JsonOutputFormat) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = "/Users/aronic/Documents/codelint/tests/test_check_init.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file;
  FILE* pipe = popen(cmd.c_str(), "r");
  ASSERT_NE(pipe, nullptr);

  char buffer[4096];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  EXPECT_TRUE(output.find("issues") != std::string::npos);
}

// Test version output
TEST_F(IntegrationTest, VersionOutput) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string cmd = codelint_path + " --version";
  FILE* pipe = popen(cmd.c_str(), "r");
  ASSERT_NE(pipe, nullptr);

  char buffer[1024];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  EXPECT_TRUE(output.find("codelint") != std::string::npos);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
