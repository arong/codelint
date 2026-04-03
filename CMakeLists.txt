cmake_minimum_required(VERSION 3.20)
project(codelint VERSION 1.0.0 LANGUAGES C CXX)

# ========== C++ Standard Detection (C++26/C++23 with C++20 fallback) ==========
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++23" COMPILER_SUPPORTS_CXX23)

if(COMPILER_SUPPORTS_CXX23)
    set(CMAKE_CXX_STANDARD 23)
    message(STATUS "Using C++23")
else()
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "Using C++20 (fallback)")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Build Options ==========
option(STATIC_LINKING "Build with static linking for release" OFF)
option(COVERAGE "Enable code coverage (use: cmake -DCOVERAGE=ON .. && make coverage)" OFF)

# ========== Coverage Configuration ==========
if(COVERAGE)
    message(STATUS "Coverage enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# ========== LLVM/Clang Configuration (Portable) ==========
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# LLVM include directories and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# LLVM components required for LibTooling
llvm_map_components_to_libnames(llvm_libs
    support
    core
    analysis
    clang
    clang-frontend
    clang-tooling
    clang-ast
    clang-basic
    clang-lex
    clang-parse
    clang-sema
    clang-codegen
)

# ========== Project Configuration ==========
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/include)
# Use system rapidjson from brew
find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h HINTS /opt/homebrew/include)
if(RAPIDJSON_INCLUDE_DIR)
    message(STATUS "Found RapidJSON at: ${RAPIDJSON_INCLUDE_DIR}")
    include_directories(${RAPIDJSON_INCLUDE_DIR})
endif()

# ========== libgit2 Configuration ==========
find_package(libgit2 REQUIRED)
message(STATUS "Found libgit2 ${libgit2_VERSION}")

# ========== Executable Definition ==========
add_executable(codelint
  src/main.cpp
  src/commands/cmd_utils.cpp
  src/commands/cmd_check_init.cpp
  src/commands/cmd_find_global.cpp
  src/commands/cmd_find_singleton.cpp
  src/lint.cpp
  src/lint_checker.cpp
  src/issue_reporter.cpp
  src/fix_applier.cpp
  src/lint/lint_runner.cpp
  src/lint/lint_action.cpp
  src/lint/git_scope.cpp
  src/lint_visitor.cpp
  src/lint/checkers/init_checker.cpp
  src/lint/checkers/const_checker.cpp
  src/lint/checkers/global_checker.cpp
  src/lint/checkers/singleton_checker.cpp
)

target_include_directories(codelint PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/include
)

# ========== Version Definitions ==========
target_compile_definitions(codelint PRIVATE
    CODELINT_VERSION="${PROJECT_VERSION}"
    LLVM_VERSION="${LLVM_PACKAGE_VERSION}"
)

# ========== Linkage ==========
# We need both libclang (C API for init_checker) and libclang-cpp (C++ API for other checkers)
# Use -Wl,--no-as-needed to ensure both are linked, but link libclang-cpp FIRST
# to have its LLVM command-line options registered first
if(STATIC_LINKING)
    set(LLVM_LINK_FLAGS "-static")
    separate_arguments(LLVM_LINK_FLAGS_LIST UNIX_COMMAND "${LLVM_LINK_FLAGS}")
    target_link_libraries(codelint PRIVATE ${llvm_libs} ${LLVM_LINK_FLAGS_LIST})
else()
    # Cross-platform library linking
    if(APPLE)
    target_link_libraries(codelint PRIVATE
        ${LLVM_LIBRARY_DIR}/libclang-cpp.dylib
        ${LLVM_LIBRARY_DIR}/libclang.dylib
        ${LLVM_LIBRARY_DIR}/libLLVM.dylib
        libgit2::libgit2package
    )
    else()
        target_link_libraries(codelint PRIVATE
            ${LLVM_LIBRARY_DIR}/libclang-cpp.so.18.1
            ${LLVM_LIBRARY_DIR}/libclang.so
            ${LLVM_LIBRARY_DIR}/libLLVM.so
        )
    endif()
    # Suppress LLVM command-line option duplication warning
    target_compile_options(codelint PRIVATE -DLLVM_ENABLE_DUMP=0)
endif()

# ========== Install ==========
install(TARGETS codelint DESTINATION bin)

# ========== GoogleTest Setup ==========
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Test executables
add_executable(commands_test tests/commands_test.cpp)
target_link_libraries(commands_test PRIVATE
    gtest_main
    pthread
)
target_include_directories(commands_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(commands_test PRIVATE TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME commands_test COMMAND commands_test)

add_executable(edge_cases_test tests/edge_cases_test.cpp)
target_link_libraries(edge_cases_test PRIVATE
    gtest_main
    pthread
)
target_include_directories(edge_cases_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME edge_cases_test COMMAND edge_cases_test)

add_executable(integration_test tests/integration_test.cpp)
target_link_libraries(integration_test PRIVATE
    gtest_main
    pthread
)
target_include_directories(integration_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME integration_test COMMAND integration_test)
