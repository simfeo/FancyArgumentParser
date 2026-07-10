# Standalone test build for FancyArgumentParser.
#
# This lives entirely under tests/ and is never referenced by the library
# header, so consumers who only want "argparse.h" can ignore it completely.
#
# Usage:
#     cmake -S tests -B build
#     cmake --build build
#     ctest --test-dir build --output-on-failure
cmake_minimum_required(VERSION 3.14)
project(FancyArgumentParserTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(test_argparse test_argparse.cpp test_namespace.cpp)

# The single-header library sits one directory up.
target_include_directories(test_argparse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(MSVC)
    target_compile_options(test_argparse PRIVATE /W4)
else()
    target_compile_options(test_argparse PRIVATE -Wall -Wextra)
endif()

add_test(NAME test_argparse COMMAND test_argparse)
