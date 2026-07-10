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

// A flag (0 values) records presence/absence; optional flags need SetRequired(false).
static void test_flag_presence()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("v", "verbose")
        .SetArgumentIsFlag().SetRequired(false));

    auto present = parser.ParseArgs(std::vector<std::string>{ "-v" });
    CHECK(present.IsArgValid());
    CHECK(present.GetArg("verbose").GetArgumentExists());

    auto absent = parser.ParseArgs(std::vector<std::string>{});
    CHECK(absent.IsArgValid()); // optional: omitting it is still valid
    CHECK(!absent.GetArg("verbose").GetArgumentExists());
}

// An omitted optional argument falls back to its SetDefault value.
static void test_default_value()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("j", "jobs", 1,
        argparse::ArgTypeCast::e_int, false).SetDefault(4));

    auto obj = parser.ParseArgs(std::vector<std::string>{});
    CHECK(obj.IsArgValid());
    auto jobs = obj.GetArg("jobs");
    CHECK(jobs.GetArgumentExists());
    CHECK(jobs.GetAsInt() == 4);
}

// Every argument is required by default; omitting a plain optional's value must fail.
static void test_required_by_default()
{
    auto parser = argparse::ArgumentParser("prog");
    // No SetRequired(false): this named argument is required by default.
    parser.AddArgument(argparse::CreateNamedArgument("n", "name", 1,
        argparse::ArgTypeCast::e_String));

    auto obj = parser.ParseArgs(std::vector<std::string>{});
    CHECK(!obj.IsArgValid());
}

// Two separate positional arguments each receive one token.
static void test_multiple_positionals()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreatePositionalArgument("source"));
    parser.AddArgument(argparse::CreatePositionalArgument("dest"));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "a.txt", "b.txt" });
    CHECK(obj.IsArgValid());
    auto src = obj.GetArg("source");
    auto dst = obj.GetArg("dest");
    CHECK(src.GetAsString() == "a.txt");
    CHECK(dst.GetAsString() == "b.txt");
}

// Positional and named arguments combine in a single parser.
static void test_mixed_positional_and_named()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreatePositionalArgument("path"));
    parser.AddArgument(argparse::CreateNamedArgument("f", "force")
        .SetArgumentIsFlag().SetRequired(false));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "file.txt", "-f" });
    CHECK(obj.IsArgValid());
    auto path = obj.GetArg("path");
    CHECK(path.GetAsString() == "file.txt");
    CHECK(obj.GetArg("force").GetArgumentExists());
}

// kAnyArgCount accepts zero or more values.
static void test_any_arg_count()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("n", "nums",
        argparse::kAnyArgCount, argparse::ArgTypeCast::e_int, false));

    auto many = parser.ParseArgs(std::vector<std::string>{ "--nums", "1", "2", "3" });
    CHECK(many.IsArgValid());
    CHECK(many.GetArg("nums").GetAsVecInt().size() == 3);

    auto none = parser.ParseArgs(std::vector<std::string>{});
    CHECK(none.IsArgValid()); // zero values is allowed
}

// A custom prefix character replaces the default '-'.
static void test_custom_prefix()
{
    auto parser = argparse::ArgumentParser("prog").SetPrefixChars('+');
    parser.AddArgument(argparse::CreateNamedArgument("n", "num", 1,
        argparse::ArgTypeCast::e_int, true));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "++num", "5" });
    CHECK(obj.IsArgValid());
    CHECK(obj.GetArg("num").GetAsInt() == 5);
}

// Unknown options are tolerated when SetIgnoreUknownArgs(true).
static void test_ignore_unknown_args()
{
    auto parser = argparse::ArgumentParser("prog").SetIgnoreUknownArgs(true);
    parser.AddArgument(argparse::CreateNamedArgument("n", "num", 1,
        argparse::ArgTypeCast::e_int, false));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "--num", "1", "--bogus", "2" });
    CHECK(obj.IsArgValid());
    CHECK(obj.GetArg("num").GetAsInt() == 1);
}

// bool arguments accept the documented spellings and reject others.
static void test_bool_parsing()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("b", "bits",
        argparse::kFromOneToInfinteArgCount, argparse::ArgTypeCast::e_bool, false));

    auto ok = parser.ParseArgs(std::vector<std::string>{ "--bits", "true", "False", "TRUE" });
    CHECK(ok.IsArgValid());
    auto bits = ok.GetArg("bits");
    const std::vector<bool>& v = bits.GetAsVecBool();
    CHECK(v.size() == 3);
    CHECK(v[0] == true && v[1] == false && v[2] == true);

    auto bad = parser.ParseArgs(std::vector<std::string>{ "--bits", "1" }); // "1" is not a bool
    CHECK(!bad.IsArgValid());
}

// Guards the custom-namespace macro (ARGPARSE_NAMESPACE_NAME). Defined in a
// separate translation unit so the macro can be set before the header is included.
bool run_custom_namespace_test();
static void test_custom_namespace()
{
    CHECK(run_custom_namespace_test());
}

// Guards multi-TU inclusion: defined in test_second_tu.cpp, which includes the
// header in the default namespace. Linking both TUs proves the free factory
// functions have inline linkage (no duplicate-symbol errors).
bool run_second_tu_build_check();
static void test_multi_translation_unit()
{
    CHECK(run_second_tu_build_check());
}

// Regression: long long + choices used to read the wrong (empty) vector and crash.
static void test_longlong_choices()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("x", "xval", 1,
        argparse::ArgTypeCast::e_longlong, true)
        .SetChoices(std::vector<long long>{ 10, 20, 30 }));

    auto ok = parser.ParseArgs(std::vector<std::string>{ "--xval", "20" });
    CHECK(ok.IsArgValid());
    CHECK(ok.GetArg("xval").GetAsLongLong() == 20);

    auto bad = parser.ParseArgs(std::vector<std::string>{ "--xval", "25" });
    CHECK(!bad.IsArgValid()); // 25 is out of choices
}

// Reference-returning getters now return by value, so calling them directly on
// the temporary from GetArg(...) no longer dangles.
static void test_getters_do_not_dangle()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreatePositionalArgument("name"));
    parser.AddArgument(argparse::CreateNamedArgument("n", "nums",
        argparse::kFromOneToInfinteArgCount, argparse::ArgTypeCast::e_int, false));

    auto obj = parser.ParseArgs(std::vector<std::string>{ "hello", "--nums", "1", "2" });
    CHECK(obj.IsArgValid());

    // Bind a value straight from the temporary ArgumentParsed: previously dangling.
    std::string name = obj.GetArg("name").GetAsString();
    CHECK(name == "hello");

    std::vector<int> nums = obj.GetArg("nums").GetAsVecInt();
    CHECK(nums.size() == 2 && nums[0] == 1 && nums[1] == 2);
}

// Scalar getters throw std::out_of_range instead of invoking UB on empty access.
static void test_getter_throws_when_empty()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("n", "num", 1,
        argparse::ArgTypeCast::e_int, false));

    auto obj = parser.ParseArgs(std::vector<std::string>{}); // num absent
    auto num = obj.GetArg("num");
    CHECK(!num.GetArgumentExists());

    bool threw = false;
    try { (void)num.GetAsInt(); }
    catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
}

// Correctly-spelled aliases behave like the original misspelled API.
static void test_spelling_aliases()
{
    // kFromOneToInfiniteArgCount == kFromOneToInfinteArgCount
    CHECK(argparse::kFromOneToInfiniteArgCount == argparse::kFromOneToInfinteArgCount);

    auto parser = argparse::ArgumentParser("prog").SetIgnoreUnknownArgs(true);
    parser.AddArgument(argparse::CreateNamedArgument("n", "num", 1,
        argparse::ArgTypeCast::e_int, false));
    auto obj = parser.ParseArgs(std::vector<std::string>{ "--num", "1", "--bogus", "2" });
    CHECK(obj.IsArgValid());
}

// SetUsage overrides the auto-generated usage line (previously ignored).
static void test_set_usage_override()
{
    auto parser = argparse::ArgumentParser("tool").SetDescription("d")
        .SetUsage("tool [OPTIONS] FILE");
    parser.AddArgument(argparse::CreateNamedArgument("n", "name", 1,
        argparse::ArgTypeCast::e_String, false));

    std::string help = parser.GetHelp(80);
    CHECK(help.find("tool [OPTIONS] FILE") != std::string::npos);
    // The auto-generated form must not leak through.
    CHECK(help.find("tool -n,--name") == std::string::npos);
}

// A long-name-only argument renders without a stray leading comma.
static void test_long_only_no_leading_comma()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("", "verbose", 0,
        argparse::ArgTypeCast::e_String, false));

    std::string help = parser.GetHelp(80);
    CHECK(help.find("--verbose") != std::string::npos);
    CHECK(help.find(",--verbose") == std::string::npos);
}

// A bool argument stores and returns the parsed value.
static void test_bool_scalar()
{
    auto parser = argparse::ArgumentParser("prog");
    parser.AddArgument(argparse::CreateNamedArgument("d", "debug", 1,
        argparse::ArgTypeCast::e_bool, false));

    auto on = parser.ParseArgs(std::vector<std::string>{ "--debug", "true" });
    CHECK(on.IsArgValid());
    CHECK(on.GetArg("debug").GetAsBool() == true);

    auto off = parser.ParseArgs(std::vector<std::string>{ "--debug", "False" });
    CHECK(off.IsArgValid());
    CHECK(off.GetArg("debug").GetAsBool() == false);
}

// Help text no longer glues tokens together or leaves trailing spaces on lines.
static void test_help_formatting()
{
    auto parser = argparse::ArgumentParser("prog").SetDescription("desc");
    parser.AddArgument(argparse::CreateNamedArgument("v", "verbose")
        .SetArgumentIsFlag().SetRequired(false));
    parser.AddArgument(argparse::CreateNamedArgument("j", "jobs", 1,
        argparse::ArgTypeCast::e_int, false));

    std::string help = parser.GetHelp(80);

    // No line ends with a space (i.e. no " \n" and no trailing space at the end).
    CHECK(help.find(" \n") == std::string::npos);
    CHECK(!(help.size() && help.back() == ' '));

    // Adjacent option tokens are space-separated, not glued (e.g. "]-" or "e[").
    CHECK(help.find("][") == std::string::npos);
    CHECK(help.find("verbose[") == std::string::npos);
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
    RUN(test_flag_presence);
    RUN(test_default_value);
    RUN(test_required_by_default);
    RUN(test_multiple_positionals);
    RUN(test_mixed_positional_and_named);
    RUN(test_any_arg_count);
    RUN(test_custom_prefix);
    RUN(test_ignore_unknown_args);
    RUN(test_bool_parsing);
    RUN(test_custom_namespace);
    RUN(test_multi_translation_unit);
    RUN(test_longlong_choices);
    RUN(test_getters_do_not_dangle);
    RUN(test_getter_throws_when_empty);
    RUN(test_spelling_aliases);
    RUN(test_set_usage_override);
    RUN(test_long_only_no_leading_comma);
    RUN(test_bool_scalar);
    RUN(test_help_formatting);

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks
              << " checks passed." << std::endl;

    if (g_failures != 0) {
        std::cerr << g_failures << " check(s) FAILED." << std::endl;
        return 1;
    }
    std::cout << "All tests passed." << std::endl;
    return 0;
}
