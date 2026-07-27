// C++20 module interface for the single-header argparse library.
//
// This is a thin wrapper: argparse.h is included in the global module
// fragment (so it compiles exactly as it does for #include users, with all
// internal helpers kept private), and only the public API is re-exported.
//
// Usage:   import argparse;   instead of   #include "argparse.h"
//
// The header remains fully usable on its own; this file is only needed by
// consumers who want the module form and toolchains that support C++20
// modules (MSVC, recent GCC/Clang). Define ARGPARSE_NAMESPACE_NAME before
// building this unit to change the exported namespace, same as the header.

module;

#include "argparse.h"

export module argparse;

#ifndef ARGPARSE_NAMESPACE_NAME
#define ARGPARSE_NAMESPACE_NAME argparse
#endif

export namespace ARGPARSE_NAMESPACE_NAME
{
    // Sentinel argument-count constants.
    using ARGPARSE_NAMESPACE_NAME::kAnyArgCount;
    using ARGPARSE_NAMESPACE_NAME::kFromOneToInfiniteArgCount;
    using ARGPARSE_NAMESPACE_NAME::kZeroOrOneArgCount;

    // Enums and value types.
    using ARGPARSE_NAMESPACE_NAME::ArgTypeCast;
    using ARGPARSE_NAMESPACE_NAME::NArgs;
    using ARGPARSE_NAMESPACE_NAME::ArgName;

    // Argument specification helpers.
    using ARGPARSE_NAMESPACE_NAME::NamedArgSpec;
    using ARGPARSE_NAMESPACE_NAME::PositionalArgSpec;
    using ARGPARSE_NAMESPACE_NAME::ParserSpec;

    // Core classes.
    using ARGPARSE_NAMESPACE_NAME::Argument;
    using ARGPARSE_NAMESPACE_NAME::ArgumentParsed;
    using ARGPARSE_NAMESPACE_NAME::ArgumentsObject;
    using ARGPARSE_NAMESPACE_NAME::ArgumentParser;

    // Free factory functions (all overloads).
    using ARGPARSE_NAMESPACE_NAME::CreateNamedArgument;
    using ARGPARSE_NAMESPACE_NAME::CreatePositionalArgument;
}
