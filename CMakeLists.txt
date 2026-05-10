cmake_minimum_required(VERSION 3.20)
project(codelint-core VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Coverage support option
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

# Singleton check option (disabled for CI/release builds)
option(ENABLE_SINGLETON_CHECK "Enable Meyer's Singleton pattern detection check" ON)

if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage instrumentation")
    # Clang coverage flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    # Linker flags for coverage
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-instr-generate")
endif()

# Find LLVM/Clang
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set Clang_DIR based on LLVM_DIR
if(NOT DEFINED Clang_DIR)
  get_filename_component(LLVM_DIR_PARENT "${LLVM_DIR}" DIRECTORY)
  set(Clang_DIR "${LLVM_DIR_PARENT}/clang")
endif()
find_package(Clang REQUIRED CONFIG)
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# LLVM version detection for compatibility
message(STATUS "LLVM version major: ${LLVM_VERSION_MAJOR}")
if(LLVM_VERSION_MAJOR VERSION_LESS 15)
  message(FATAL_ERROR "codelint requires LLVM 15 or higher, found ${LLVM_VERSION_MAJOR}")
endif()

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${Clang_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Use LLVM's cmake modules
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Plugin shared library
add_llvm_library(codelint-core SHARED
    src/codelint_plugin/CodelintModule.cpp
    src/codelint_plugin/utils/InitUtils.cpp
    src/codelint_plugin/checks/InitCheck.cpp
    src/codelint_plugin/checks/LintCodeCheck.cpp
    src/codelint_plugin/checks/LocalVarNamingCheck.cpp
    src/codelint_plugin/checks/FunctionSizeCheck.cpp
    src/codelint_plugin/checks/GlobalConstStringCheck.cpp
    src/codelint_plugin/checks/SingletonCheck.cpp
    src/codelint_plugin/checks/StrictBoolConditionCheck.cpp
    src/codelint_plugin/checks/SignedToUnsignedReturnCheck.cpp
    src/codelint_plugin/checks/LogTagMismatchCheck.cpp

    LINK_COMPONENTS
    support
)

target_include_directories(codelint-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Pass LLVM version for macro-based compatibility
target_compile_definitions(codelint-core PRIVATE
    CODELINT_LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
)

if(NOT ENABLE_SINGLETON_CHECK)
    message(STATUS "Disabling singleton pattern detection check")
    target_compile_definitions(codelint-core PRIVATE
        CODELINT_DISABLE_SINGLETON_CHECK
    )
endif()

# Link against clang libraries
target_link_libraries(codelint-core PRIVATE
    clangTidy
    clangTidyPlugin
    clangAST
    clangASTMatchers
    clangBasic
    clangLex
    clangFrontend
    clangTooling
    clangSerialization
    clangSema
    clangAnalysis
    clangDriver
    clangEdit
    clangParse
)

# Set plugin properties
set_target_properties(codelint-core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "codelint-core"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

install(TARGETS codelint-core LIBRARY DESTINATION lib/clang-tidy)

add_custom_target(test-lit
    COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_lit_tests.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS codelint-core
    COMMENT "Running lit-style tests"
    VERBATIM
)
