cmake_minimum_required(VERSION 3.14)
project(cndy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(cndy
  src/main.cpp
  src/find_global.cpp
  src/find_singleton.cpp
  src/check_init.cpp
)

target_include_directories(cndy PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/include
)

install(TARGETS cndy DESTINATION bin)