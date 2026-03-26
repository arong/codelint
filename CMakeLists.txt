cmake_minimum_required(VERSION 3.14)
project(cndy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_library(CLANG_LIB NAMES clang PATHS /usr/lib/llvm-18/lib /usr/lib /usr/lib/x86_64-linux-gnu)

if(NOT CLANG_LIB)
  message(FATAL_ERROR "Could not find libclang library")
endif()

add_executable(cndy
  src/main.cpp
  src/find_global.cpp
  src/find_singleton.cpp
  src/check_init.cpp
  src/lint.cpp
  src/lint_checker.cpp
  src/lint/lint_runner.cpp
  src/lint/checkers/init_checker.cpp
  src/lint/checkers/const_checker.cpp
  src/lint/checkers/global_checker.cpp
  src/lint/checkers/singleton_checker.cpp
)

target_include_directories(cndy PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include
  /usr/lib/llvm-18/include
)

target_link_libraries(cndy PRIVATE ${CLANG_LIB})

install(TARGETS cndy DESTINATION bin)