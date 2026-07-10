// Verifies the ARGPARSE_NAMESPACE_NAME customization point. The macro must be
// defined BEFORE the header is included, so this lives in its own translation
// unit and is linked into the main test executable.
//
// This also guards against a regression where internal code hardcoded the
// default "argparse::" qualifier, which broke any custom namespace.

#define ARGPARSE_NAMESPACE_NAME cli
#include <string>
#include <vector>

#include "argparse.h"

bool run_custom_namespace_test()
{
    auto parser = cli::ArgumentParser("greet");
    parser.AddArgument(cli::CreateNamedArgument("n", "name", 1,
        cli::ArgTypeCast::e_String, true));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "--name", "World" });
    if (!obj.IsArgValid())
        return false;

    auto name = obj.GetArg("name");
    return name.GetAsString() == "World";
}
