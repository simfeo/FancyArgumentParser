// Smoke test for the C++20 module form of the library (argparse.ixx).
//
// Built only where the toolchain supports modules (see tests/CMakeLists.txt);
// it exercises the same public API as the header tests but reaches it through
// `import argparse;` instead of `#include "argparse.h"`.

import argparse;

#include <vector>
#include <string>
#include <iostream>

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL: " #cond " (" << __FILE__ << ":"            \
                      << __LINE__ << ")\n";                                \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

int main()
{
    argparse::ArgumentParser parser("demo");
    parser.AddArgument(argparse::CreateNamedArgument("n", "numbers")
        .SetAnyNumberOfArgumentsButAtLeastOne()
        .SetType(argparse::ArgTypeCast::e_int));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "-n", "1", "2", "3" });
    CHECK(obj.IsArgValid());

    const std::vector<int>& v = obj.GetArg("numbers").GetAsVecInt();
    CHECK(v.size() == 3);
    CHECK(v.at(0) == 1);
    CHECK(v.at(2) == 3);

    // Re-exported sentinel constant is visible through the module.
    int anyCount = argparse::kAnyArgCount;
    CHECK(anyCount == -1);

    if (failures == 0)
        std::cout << "module test: all checks passed.\n";
    return failures == 0 ? 0 : 1;
}
