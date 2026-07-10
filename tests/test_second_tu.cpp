// Regression guard for multi-translation-unit inclusion.
//
// This TU includes argparse.h in the DEFAULT namespace and uses the free
// factory functions, just like test_argparse.cpp does. If those functions ever
// lose their `inline` linkage again, linking this file together with
// test_argparse.cpp fails with a duplicate-symbol error -- catching the
// regression at build time.

#include "argparse.h"

bool run_second_tu_build_check()
{
    auto parser = argparse::ArgumentParser("second");
    parser.AddArgument(argparse::CreateNamedArgument("n", "num", 1,
        argparse::ArgTypeCast::e_int, false));
    parser.AddArgument(argparse::CreatePositionalArgument("pos"));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "hello", "--num", "1" });
    auto pos = obj.GetArg("pos");
    return obj.IsArgValid() && pos.GetAsString() == "hello";
}
