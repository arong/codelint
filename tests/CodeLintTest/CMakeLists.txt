cmake_minimum_required(VERSION 3.14)
project(CodeLintTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TEST_SOURCES
    src/init_check.cpp
    src/test_find_global.cpp
    src/test_find_singleton.cpp
    src/init_checker/src/bitfield.cpp
    src/init_checker/fixed/bitfield.cpp
    src/init_checker/src/builtin_types.cpp
    src/init_checker/fixed/builtin_types.cpp
    src/init_checker/src/c_array.cpp
    src/init_checker/fixed/c_array.cpp
    src/init_checker/src/container_types.cpp
    src/init_checker/fixed/container_types.cpp
    src/init_checker/src/default_argument.cpp
    src/init_checker/fixed/default_argument.cpp
    src/init_checker/src/enum_types.cpp
    src/init_checker/fixed/enum_types.cpp
    src/init_checker/src/exception.cpp
    src/init_checker/fixed/exception.cpp
    src/init_checker/src/global_scope.cpp
    src/init_checker/fixed/global_scope.cpp
    src/init_checker/src/integer.cpp
    src/init_checker/fixed/integer.cpp
    src/init_checker/src/local_variables.cpp
    src/init_checker/fixed/local_variables.cpp
    src/init_checker/src/macro_array.cpp
    src/init_checker/fixed/macro_array.cpp
    src/init_checker/src/member_init.cpp
    src/init_checker/fixed/member_init.cpp
    src/init_checker/src/multi_declarator.cpp
    src/init_checker/fixed/multi_declarator.cpp
    src/init_checker/src/pointer_types.cpp
    src/init_checker/fixed/pointer_types.cpp
    src/init_checker/src/reference.cpp
    src/init_checker/fixed/reference.cpp
    src/init_checker/src/special_types.cpp
    src/init_checker/fixed/special_types.cpp
    src/init_checker/src/std.cpp
    src/init_checker/fixed/std.cpp
    src/init_checker/src/struct_class.cpp
    src/init_checker/fixed/struct_class.cpp
    src/init_checker/src/type_aliases.cpp
    src/init_checker/fixed/type_aliases.cpp
    src/global_checker/src/anon_namespace_globals.cpp
    src/global_checker/src/array_globals.cpp
    src/global_checker/src/basic_globals.cpp
    src/global_checker/src/class_globals.cpp
    src/global_checker/src/class_member.cpp
    src/global_checker/src/const_globals.cpp
    src/global_checker/src/extern_globals.cpp
    src/global_checker/src/function_local_static.cpp
    src/global_checker/src/inline_globals.cpp
    src/global_checker/src/namespace_globals.cpp
    src/global_checker/src/static_globals.cpp
    src/global_checker/src/template_globals.cpp
    src/global_checker/src/thread_local_globals.cpp
    src/global_checker/src/typed_globals.cpp
    src/singleton_checker/src/false_positive_pointer.cpp
    src/singleton_checker/src/false_positive_ref.cpp
    src/singleton_checker/src/false_positive_static.cpp
    src/singleton_checker/src/false_positive_value.cpp
    src/singleton_checker/src/getinstance_singleton.cpp
    src/singleton_checker/src/meyers_singleton.cpp
    src/singleton_checker/src/multi_singleton.cpp
    src/singleton_checker/src/namespace_singleton.cpp
    src/singleton_checker/src/no_return_static.cpp
    src/singleton_checker/src/param_singleton.cpp
    src/singleton_checker/src/template_singleton.cpp
    src/signed_to_unsigned_checker/src/signed_to_unsigned.cpp
    src/signed_to_unsigned_checker/fixed/signed_to_unsigned.cpp
    src/null_deref_checker/src/basic_null_deref.cpp
    src/null_deref_checker/src/control_flow_null.cpp
    src/null_deref_checker/src/member_pointer_null.cpp
    src/null_deref_checker/src/advanced_null_patterns.cpp
    src/null_deref_checker/src/false_positive_null.cpp
    src/null_deref_checker/src/edge_cases_null.cpp
    src/null_deref_checker/src/lifecycle_pointer.cpp
    src/null_deref_checker/src/workflow_api_patterns.cpp
    src/null_deref_checker/src/exception_safety_patterns.cpp
    src/null_deref_checker/src/null_deref_test.cpp
)

add_library(codelint_test_objects OBJECT ${TEST_SOURCES})

target_include_directories(codelint_test_objects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
