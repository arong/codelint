cmake_minimum_required(VERSION 3.20)

project(codelint_lit_tests_v2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/lit.cfg.py
)

find_program(LLVM-lit lit.py)
if(NOT LLVM-lit)
    find_program(LLVM-lit llvmlit.py)
endif()

if(LLVM-lit)
    add_custom_target(check-codelint-lit-v2
        COMMAND ${LLVM-lit} -v ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running codelint lit-style tests v2"
        DEPENDS codelint-plugin
    )
endif()

add_custom_target(lit-test-v2 ALL
    COMMAND chmod +x ${CMAKE_CURRENT_SOURCE_DIR}/check_codelint.py
)