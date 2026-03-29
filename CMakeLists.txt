cmake_minimum_required(VERSION 3.20)
project(codelint VERSION 1.0.0 LANGUAGES CXX)

# ========== C++ Standard Detection (C++26/C++23 with C++20 fallback) ==========
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++26" COMPILER_SUPPORTS_CXX26)
check_cxx_compiler_flag("-std=c++23" COMPILER_SUPPORTS_CXX23)

if(COMPILER_SUPPORTS_CXX26)
    set(CMAKE_CXX_STANDARD 26)
    message(STATUS "Using C++26")
elseif(COMPILER_SUPPORTS_CXX23)
    set(CMAKE_CXX_STANDARD 23)
    message(STATUS "Using C++23")
else()
    set(CMAKE_CXX_STANDARD 20)
    message(STATUS "Using C++20 (fallback)")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== Build Options ==========
option(STATIC_LINKING "Build with static linking for release" OFF)

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include)

# ========== Executable Definition ==========
add_executable(codelint
  src/main.cpp
  src/lint.cpp
  src/lint_checker.cpp
  src/issue_reporter.cpp
  src/fix_applier.cpp
  src/lint/lint_runner.cpp
  src/lint/lint_action.cpp
  src/lint_visitor.cpp
  src/lint/checkers/init_checker.cpp
  src/lint/checkers/const_checker.cpp
  src/lint/checkers/global_checker.cpp
  src/lint/checkers/singleton_checker.cpp
)

target_include_directories(codelint PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include
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
    # Link libclang-cpp first (C++ API), then libclang (C API)
    # Use LINK_PRIVATE to avoid symbol export issues
    target_link_libraries(codelint PRIVATE 
        ${LLVM_LIBRARY_DIR}/libclang-cpp.so.18.1
        ${LLVM_LIBRARY_DIR}/libclang.so
        ${LLVM_LIBRARY_DIR}/libLLVM.so
    )
    # Suppress LLVM command-line option duplication warning
    target_compile_options(codelint PRIVATE -DLLVM_ENABLE_DUMP=0)
endif()

# ========== Install ==========
install(TARGETS codelint DESTINATION bin)