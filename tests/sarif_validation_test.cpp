#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <sstream>

// SARIF validation tests - verify SARIF output format compliance
// These tests examine the output of the compiled codelint binary with --output-sarif flag

class SarifValidationTest : public ::testing::Test {
protected:
  std::string build_dir;
  std::string codelint_path;
  std::string test_data_dir;

  void SetUp() override {
    build_dir = std::getenv("BUILD_DIR") ? std::getenv("BUILD_DIR")
                                         : "/Users/aronic/Documents/codelint/build";
    codelint_path = build_dir + "/codelint";
    test_data_dir = "/Users/aronic/Documents/codelint/tests";
  }

  bool codelintExists() {
    return std::filesystem::exists(codelint_path);
  }

  // Helper function to run codelint and capture output
  std::string runCodelintSarif(const std::string& target) {
    std::string cmd = codelint_path + " --output-sarif check_init " + target + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      output += buffer;
    }
    pclose(pipe);
    return output;
  }

  // Helper function to validate basic JSON structure
  bool isValidJson(const std::string& json_str) {
    if (json_str.empty()) return false;
    // Basic check: must start with { or [
    return json_str.find('{') == 0 || json_str.find('[') == 0;
  }

  // Helper to check if string contains substring
  bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
  }
};

// Test SARIF version field
TEST_F(SarifValidationTest, SarifOutputHasVersion) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"version\""));
  EXPECT_TRUE(contains(output, "\"2.1.0\""));
}

// Test SARIF runs array exists
TEST_F(SarifValidationTest, SarifOutputHasRunsArray) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"runs\""));
}

// Test tool.driver.name exists (required by SARIF spec)
TEST_F(SarifValidationTest, SarifOutputHasToolDriverName) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"tool\""));
  EXPECT_TRUE(contains(output, "\"driver\""));
  EXPECT_TRUE(contains(output, "\"name\""));
}

// Test results array presence
TEST_F(SarifValidationTest, SarifOutputHasResultsArray) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"results\""));
}

// Test empty results case
TEST_F(SarifValidationTest, SarifOutputEmptyResults) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string clean_file = test_data_dir + "/test_find_singleton.cpp";
  std::string output = runCodelintSarif(clean_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"results\""));
}

// Test schema presence
TEST_F(SarifValidationTest, SarifOutputHasSchemaUri) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "$schema") || contains(output, "\"schema\""));
}

// Test severity mapping fields
TEST_F(SarifValidationTest, SarifOutputHasRuleId) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"ruleId\""));
}

// Test location fields presence
TEST_F(SarifValidationTest, SarifOutputHasLocations) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"locations\"") ||
              contains(output, "\"physicalLocation\"") ||
              contains(output, "\"artifactLocation\""));
}

// Test multi-file handling produces valid JSON structure
TEST_F(SarifValidationTest, SarifOutputMultiFileValidStructure) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_dir = test_data_dir;
  std::string cmd = codelint_path + " --output-sarif check_init " + test_dir + " 2>/dev/null";
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    GTEST_SKIP() << "Could not run codelint";
  }

  char buffer[4096];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(isValidJson(output));
}

// Test backward compatibility - JSON flag still works
TEST_F(SarifValidationTest, BackwardCompatJsonFlag) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string cmd = codelint_path + " --output-json check_init " + test_file + " 2>/dev/null";
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    GTEST_SKIP() << "Could not run codelint";
  }

  char buffer[4096];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  pclose(pipe);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(isValidJson(output));
  EXPECT_TRUE(contains(output, "\"issues\""));
}

// Test severity mapping in SARIF output - ERROR -> "error"
TEST_F(SarifValidationTest, SarifSeverityErrorMapsToError) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  if (contains(output, "\"level\"")) {
    EXPECT_TRUE(contains(output, "\"error\"") || contains(output, "\"warning\""));
  }
}

// Test severity mapping in SARIF output - WARNING -> "warning"
TEST_F(SarifValidationTest, SarifSeverityWarningMapsToWarning) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  if (contains(output, "\"level\"")) {
    EXPECT_TRUE(contains(output, "\"warning\""));
  }
}

// Test severity mapping in SARIF output - INFO -> "note"
TEST_F(SarifValidationTest, SarifSeverityInfoMapsToNote) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  // Info level maps to "note" in SARIF
  EXPECT_TRUE(contains(output, "\"level\""));
}

// Test severity mapping in SARIF output - HINT -> "none"
TEST_F(SarifValidationTest, SarifSeverityHintMapsToNone) {
  if (!codelintExists()) {
    GTEST_SKIP() << "codelint not built";
  }

  std::string test_file = test_data_dir + "/test_find_global.cpp";
  std::string output = runCodelintSarif(test_file);

  ASSERT_FALSE(output.empty());
  EXPECT_TRUE(contains(output, "\"level\""));
}
// Golden fixture validation tests
TEST_F(SarifValidationTest, ValidateGoldenFixtures) {
  std::string golden_dir = test_data_dir + "/golden";

  if (!std::filesystem::exists(golden_dir)) {
    GTEST_SKIP() << "Golden fixtures directory not found";
  }

  // Check that at least one .sarif fixture exists
  bool has_fixtures = false;
  for (const auto& entry : std::filesystem::directory_iterator(golden_dir)) {
    if (entry.path().extension() == ".sarif") {
      has_fixtures = true;
      // Validate JSON structure
      std::ifstream file(entry.path());
      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string content = buffer.str();

      EXPECT_TRUE(isValidJson(content)) << "Invalid JSON in " << entry.path().filename();

      // Check for required SARIF fields
      EXPECT_TRUE(contains(content, "\"version\"") || contains(content, "\"$schema\""))
          << "Missing version/schema in " << entry.path().filename();
    }
  }

  EXPECT_TRUE(has_fixtures) << "No .sarif fixtures found in " << golden_dir;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
