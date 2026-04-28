cmake_minimum_required(VERSION 3.20)

project(codelint_lit_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Find LLVM/Clang for lit
find_package(LLVM REQUIRED CONFIG)

# lit configuration
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg.py
)

# Custom target to run lit tests
find_program(LLVM-lit lit.py)
if(NOT LLVM-lit)
    find_program(LLVM-lit llvmlit.py)
endif()

if(NOT LLVM-lit)
    message(WARNING "lit not found, test target will not be available")
else()
    add_custom_target(check-codelint-lit
        COMMAND ${LLVM-lit} ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running codelint lit-based tests"
    )
endif()

# Make check_codelint.py executable
add_custom_target(codelint-lit-test ALL
    COMMAND chmod +x ${CMAKE_CURRENT_SOURCE_DIR}/check_codelint.py
    DEPENDS codelint-plugin
)
