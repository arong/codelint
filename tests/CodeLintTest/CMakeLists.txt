cmake_minimum_required(VERSION 3.14)
project(CodeLintTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 测试源文件
set(TEST_SOURCES
    src/init_check.cpp
    src/test_find_global.cpp
    src/test_find_singleton.cpp
    src/init_checker/src/init_check.cpp
    src/init_checker/fixed/init_check.cpp
)

# 创建静态库（不实际链接，只为了生成 compile_commands.json）
add_library(codelint_test_objects OBJECT ${TEST_SOURCES})

# 设置包含目录
target_include_directories(codelint_test_objects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
