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

# Defaults to C++17 but can be overridden, e.g. -DCMAKE_CXX_STANDARD=20.
# The library supports C++11/14/17/20/23; CI exercises several of these.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_executable(test_argparse test_argparse.cpp test_namespace.cpp test_second_tu.cpp)

# The single-header library sits one directory up.
target_include_directories(test_argparse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(MSVC)
    target_compile_options(test_argparse PRIVATE /W4)
else()
    target_compile_options(test_argparse PRIVATE -Wall -Wextra)
endif()

add_test(NAME test_argparse COMMAND test_argparse)

# --- Optional C++20 module build -------------------------------------------
# Builds the same library through argparse.ixx (import argparse;) instead of the
# header. Only enabled where the toolchain reliably supports named modules:
# MSVC, or GCC >= 14, with CMake >= 3.28 and C++20 or newer. Everything else
# (e.g. AppleClang) simply skips it, so the header-only path is never affected.
set(_argparse_build_module OFF)
if(NOT CMAKE_VERSION VERSION_LESS "3.28" AND CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    if(MSVC)
        set(_argparse_build_module ON)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
           AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14")
        set(_argparse_build_module ON)
    endif()
endif()

if(_argparse_build_module)
    # Enable scanning of ordinary sources for `import` (off by default because
    # cmake_minimum_required above predates modules); needed so the consumer is
    # ordered after the module interface.
    if(POLICY CMP0155)
        cmake_policy(SET CMP0155 NEW)
    endif()
    add_executable(test_module test_module.cpp)
    target_sources(test_module
        PRIVATE
        FILE_SET argparse_module TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/../argparse.ixx)
    target_include_directories(test_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_compile_features(test_module PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(test_module PRIVATE /W4)
    else()
        target_compile_options(test_module PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME test_module COMMAND test_module)
    message(STATUS "argparse: C++20 module test enabled")
else()
    message(STATUS "argparse: C++20 module test skipped (toolchain/standard)")
endif()
