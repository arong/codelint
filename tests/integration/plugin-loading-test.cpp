#include "gtest/gtest.h"
#include <dlfcn.h>
#include <filesystem>
#include <string>

namespace {

class PluginLoadingTest : public ::testing::Test {
protected:
  void* plugin_handle = nullptr;
  std::string plugin_path;

  void SetUp() override {
    plugin_path = std::filesystem::current_path() / "lib" / "codelint-plugin.so";
  }

  void TearDown() override {
    if (plugin_handle) {
      dlclose(plugin_handle);
    }
  }
};

TEST_F(PluginLoadingTest, PluginFileExists) {
  EXPECT_TRUE(std::filesystem::exists(plugin_path)) << "Plugin not found at: " << plugin_path;
}

TEST_F(PluginLoadingTest, PluginLoadsSuccessfully) {
  if (!std::filesystem::exists(plugin_path)) {
    GTEST_SKIP() << "Plugin file not built yet";
  }

  plugin_handle = dlopen(plugin_path.c_str(), RTLD_NOW);
  EXPECT_NE(plugin_handle, nullptr) << "Failed to load plugin: " << dlerror();
}

TEST_F(PluginLoadingTest, PluginExportsModuleRegistration) {
  if (!std::filesystem::exists(plugin_path)) {
    GTEST_SKIP() << "Plugin file not built yet";
  }

  plugin_handle = dlopen(plugin_path.c_str(), RTLD_NOW);
  if (!plugin_handle) {
    GTEST_SKIP() << "Failed to load plugin";
  }

  void* symbol =
      dlsym(plugin_handle,
            "_ZN5clang4tidy27ClangTidyModuleRegistry3AddINS_7codelint13CodelintModuleEEC1EPKcS6_");
  EXPECT_NE(symbol, nullptr) << "Module registration symbol not found";
}

} // namespace