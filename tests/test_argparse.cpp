// Zero-dependency tests for FancyArgumentParser.
//
// These tests are NOT part of the library. Consumers only need "argparse.h";
// nothing here is included by the header, so it never affects their build.
//
// Build & run via CMake (see tests/CMakeLists.txt) or directly:
//     c++ -std=c++17 -I.. test_argparse.cpp -o test_argparse && ./test_argparse

#include <iostream>
#include <string>
#include <vector>

#include "argparse.h"

// --- Tiny assertion framework (no third-party dependency) ------------------

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::cerr << "FAILED: " << #cond << "\n    at " << __FILE__   \
                      << ":" << __LINE__ << std::endl;                    \
        }                                                                 \
    } while (0)

#define RUN(test)                                                         \
    do {                                                                  \
        int before = g_failures;                                         \
        test();                                                          \
        std::cout << (g_failures == before ? "[ PASS ] " : "[ FAIL ] ")  \
                  << #test << std::endl;                                  \
    } while (0)

// --- Tests -----------------------------------------------------------------

// Named argument accepting one-or-more ints is parsed into a vector.
static void test_named_int_vector()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument()
        .SetLongName("numbers")
        .SetAnyNumberOfArgumentsButAtleastOne()
        .SetType(argparse::ArgTypeCast::e_int));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "--numbers", "1", "2", "3" });
    CHECK(obj.IsArgValid());

    auto arg = obj.GetArg("numbers");
    CHECK(arg.GetArgumentExists());

    const std::vector<int>& v = arg.GetAsVecInt();
    CHECK(v.size() == 3);
    CHECK(v[0] == 1 && v[1] == 2 && v[2] == 3);
}

// The short name (-n) resolves to the same argument as the long name.
static void test_short_name_alias()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("n", "numbers",
        argparse::kFromOneToInfinteArgCount, argparse::ArgTypeCast::e_int));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "-n", "42" });
    CHECK(obj.IsArgValid());
    CHECK(obj.GetArg("numbers").GetAsVecInt().at(0) == 42);
}

// A single required value is accessible via the scalar getter.
static void test_single_required_value()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("b", "b_key", 1,
        argparse::ArgTypeCast::e_int, true));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "-b", "7" });
    CHECK(obj.IsArgValid());
    CHECK(obj.GetArg("b_key").GetAsInt() == 7);
}

// A missing required argument makes parsing invalid and yields an error string.
static void test_missing_required_is_invalid()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("b", "b_key", 1,
        argparse::ArgTypeCast::e_int, true));

    auto obj = parser.ParseArgs(std::vector<std::string>{});
    CHECK(!obj.IsArgValid());
    CHECK(!obj.GetErrorString().empty());
}

// Positional arguments are read from bare (unkeyed) tokens.
static void test_positional_argument()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreatePositionalArgument("int1")
        .SetType(argparse::ArgTypeCast::e_int)
        .SetRequired(false));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "123" });
    CHECK(obj.IsArgValid());
    auto arg = obj.GetArg("int1");
    CHECK(arg.GetArgumentExists());
    CHECK(arg.GetAsInt() == 123);
}

// A value outside the declared choices is rejected.
static void test_choices_reject_out_of_set()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("o", "operation")
        .SetRequired(true)
        .SetChoices({ "+", "-", "*", "/" }));

    auto ok = parser.ParseArgs(std::vector<std::string>{ "-o", "+" });
    CHECK(ok.IsArgValid());
    CHECK(ok.GetArg("operation").GetAsString() == "+");

    auto parser2 = argparse::ArgumentParser("prog");
    parser2.AddArgument(argparse::CreateNamedArgument("o", "operation")
        .SetRequired(true)
        .SetChoices({ "+", "-", "*", "/" }));

    auto bad = parser2.ParseArgs(std::vector<std::string>{ "-o", "%" });
    CHECK(!bad.IsArgValid());
}

// Double parsing works end to end (mirrors the polish-notation example).
static void test_positional_doubles()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreatePositionalArgument("nums")
        .SetType(argparse::ArgTypeCast::e_double)
        .SetNumberOfArguments(2));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "1.5", "2.5" });
    CHECK(obj.IsArgValid());
    auto arg = obj.GetArg("nums");
    const std::vector<double>& nums = arg.GetAsVecDouble();
    CHECK(nums.size() == 2);
    CHECK(nums[0] == 1.5 && nums[1] == 2.5);
}

// An unknown key makes parsing invalid.
static void test_unknown_argument_is_invalid()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("n", "numbers", 1,
        argparse::ArgTypeCast::e_int, false));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "--nonexistent", "1" });
    CHECK(!obj.IsArgValid());
}

// Help text can be generated without throwing.
static void test_help_generation()
{
    auto parser = argparse::ArgumentParser("prog").SetDescription("desc");
    parser.AddArgument(argparse::CreateNamedArgument("n", "numbers", 1,
        argparse::ArgTypeCast::e_int, false, "a number"));

    std::string help = parser.GetHelp(80);
    CHECK(!help.empty());
    CHECK(help.find("numbers") != std::string::npos);
}

int main()
{
    RUN(test_named_int_vector);
    RUN(test_short_name_alias);
    RUN(test_single_required_value);
    RUN(test_missing_required_is_invalid);
    RUN(test_positional_argument);
    RUN(test_choices_reject_out_of_set);
    RUN(test_positional_doubles);
    RUN(test_unknown_argument_is_invalid);
    RUN(test_help_generation);

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks
              << " checks passed." << std::endl;

    if (g_failures != 0) {
        std::cerr << g_failures << " check(s) FAILED." << std::endl;
        return 1;
    }
    std::cout << "All tests passed." << std::endl;
    return 0;
}
