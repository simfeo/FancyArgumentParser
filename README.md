<div align="center">

# FancyArgumentParser

**A single-header, dependency-free C++ command-line argument parser.**

Argparse-style ergonomics for C++ — named & positional arguments, type casting,
choices, value validators, defaults, auto-generated help, and Python-like
keyword arguments in C++20.

[![tests](https://github.com/simfeo/FancyArgumentParser/actions/workflows/tests.yml/badge.svg)](https://github.com/simfeo/FancyArgumentParser/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](#license)
![C++](https://img.shields.io/badge/C%2B%2B-11%2F14%2F17%2F20%2F23-blue.svg)
![header-only](https://img.shields.io/badge/header--only-single%20file-brightgreen.svg)

[Tutorial](https://github.com/simfeo/FancyArgumentParser/wiki/Tutorial) ·
[Examples](https://github.com/simfeo/FancyArgumentParser/wiki/Examples) ·
[Class reference](https://github.com/simfeo/FancyArgumentParser/wiki/Classes-description)

</div>

---

## Why

Drop one header into your project and get a fully featured argument parser — no
submodules, no CMake packages, no linking. Everything lives in `argparse.h`.

## Features

- 🧩 **Single header, zero dependencies** — just `#include "argparse.h"`.
- 📦 **C++20 module** — optional `import argparse;` via `argparse.ixx`.
- 🏷️ **Named and positional arguments**, freely mixed in any order.
- 🔢 **Typed values** — `int`, `long long`, `double`, `bool`, and `string`.
- 🎚️ **Flexible arity** — fixed counts, or Python-style `'?'` (zero-or-one),
  `'*'` (zero-or-more) and `'+'` (one-or-more), plus the matching
  `kZeroOrOneArgCount` / `kAnyArgCount` / `kFromOneToInfiniteArgCount` constants.
- ✅ **Validation** — required/optional, value `choices`, numeric `SetRange`,
  `SetPositive`, filesystem checks (`SetExistingFile`), and custom
  `SetValidator` predicates.
- 🔗 **Variable binding** — `BindTo(&var)` writes parsed values straight into your own variables.
- 🎯 **By-name getters** — `obj.GetAsInt("count")` straight off the parsed result.
- 📖 **Auto-generated help & usage**, with custom epilogue and overridable usage line.
- 🔤 **Long-option abbreviations** (`--verb` → `--verbose` when unambiguous).
- ⚙️ **Configurable** — custom prefix characters, ignore-unknown args, custom namespace.
- 🆕 **Three declaration styles**, including Python-like **keyword arguments** in C++20.
- 🧪 **Tested** across Linux, macOS and Windows on C++17/20/23.

## Quick start

Copy `argparse.h` into your project — that's the whole installation.

```cpp
#include <iostream>
#include "argparse.h"

int main(int argc, char** argv)
{
    auto parser = argparse::ArgumentParser("greet").SetDescription("A tiny greeter");

    parser.AddArgument(argparse::CreateNamedArgument("n", "name", 1,
        argparse::ArgTypeCast::e_String, true).SetHelp("Who to greet"));
    parser.AddArgument(argparse::CreateNamedArgument("c", "count", 1,
        argparse::ArgTypeCast::e_int, false).SetDefault(1).SetHelp("How many times"));

    auto obj = parser.ParseArgs(argc, argv);
    if (!obj.IsArgValid())
    {
        std::cout << obj.GetErrorString() << "\n" << parser.GetHelp(80) << std::endl;
        return 1;
    }

    auto name = obj.GetArg("name");
    for (int i = 0; i < obj.GetArg("count").GetAsInt(); ++i)
        std::cout << "Hello, " << name.GetAsString() << "!\n";
    return 0;
}
```

```console
$ c++ -std=c++17 greet.cpp -o greet
$ ./greet --name World --count 2
Hello, World!
Hello, World!
```

## Three ways to declare an argument

Pick whichever reads best — they build the same argument.

```cpp
// 1. Fluent setters
parser.AddArgument(argparse::CreateNamedArgument()
    .SetShortName("n").SetLongName("numbers")
    .SetType(argparse::ArgTypeCast::e_int)
    .SetAnyNumberOfArgumentsButAtLeastOne());

// 2. Positional factory arguments
parser.AddArgument(argparse::CreateNamedArgument("n", "numbers",
    argparse::kFromOneToInfiniteArgCount, argparse::ArgTypeCast::e_int));

// 3. Keyword style — Python-like, needs C++20 designated initializers
parser.AddArgument(argparse::CreateNamedArgument({
    .shortName = "n",
    .longName  = "numbers",
    .nargs     = argparse::kFromOneToInfiniteArgCount,
    .type      = argparse::ArgTypeCast::e_int}));
```

> **Note:** every argument is **required by default** — call `SetRequired(false)`
> (or set `required = false`) to make one optional.

Short names may be written as a single **char** (`'n'`) as well as a string
(`"n"`), and arity accepts a Python-style **char** in place of a constant:

```cpp
// 'c' short name, '?' == zero-or-one value (same as kZeroOrOneArgCount)
parser.AddArgument(argparse::CreateNamedArgument('c', "count", '?')
    .SetType(argparse::ArgTypeCast::e_int).SetDefault(1));

// '+' == one-or-more, '*' == zero-or-more
parser.AddArgument(argparse::CreatePositionalArgument("files", '+'));
```

## Binding values to variables

Instead of pulling each value out with `GetArg(name).GetAsX()`, you can bind an
argument directly to one of your variables with `BindTo(&var)`. After a
successful `ParseArgs()`, the parsed value is written straight into it.

```cpp
int         count = 1;          // initial value doubles as the default
std::string name;
std::vector<int> ids;

auto parser = argparse::ArgumentParser("prog");
parser.AddArgument(argparse::CreateNamedArgument("c", "count", 1).BindTo(&count));
parser.AddArgument(argparse::CreateNamedArgument("n", "name",  1).BindTo(&name));
parser.AddArgument(argparse::CreateNamedArgument("i", "ids")
    .SetAnyNumberOfArgumentsButAtLeastOne().BindTo(&ids));

auto obj = parser.ParseArgs(argc, argv);
if (obj.IsArgValid())
{
    // count, name and ids are already populated — no GetArg(...) calls needed.
}
```

`BindTo` works for every supported type (`bool`, `int`, `long long`, `double`,
`std::string`) and their `std::vector<>` variants, and is available in **C++11**
onward.

- **Type is inferred** from the bound variable, so you don't need a separate
  `SetType()` call (and shouldn't add one that contradicts it).
- **The bound variable must outlive** the `ParseArgs()` call.
- If an **optional argument is absent** (and has no default), its bound variable
  is left untouched — so its initial value acts as the default.
- Bindings are applied **only on a successful parse**; a failed parse never
  writes through them.

## Validating values

Beyond `choices`, an argument can carry a validator that each parsed value must
pass; a failing value makes `ParseArgs` return an invalid result with a message.

```cpp
// Numeric range (also sets the type for you)
parser.AddArgument(argparse::CreateNamedArgument('p', "port")
    .SetRange(1, 65535));

// Strictly positive
parser.AddArgument(argparse::CreateNamedArgument('r', "ratio")
    .SetType(argparse::ArgTypeCast::e_double).SetPositive());

// Filesystem checks (available when <filesystem> is, i.e. C++17+)
parser.AddArgument(argparse::CreateNamedArgument('i', "input")
    .SetExistingFile());

// Any custom predicate, with an optional error message
parser.AddArgument(argparse::CreateNamedArgument('m', "mode")
    .SetValidator([](const std::string& v){ return v == "fast" || v == "safe"; },
                  "mode must be 'fast' or 'safe'"));
```

Built-in validators include `SetRange` (int / long long / double, as `(lo, hi)`
or `(max)`), `SetPositive`, `SetNonNegative`, and the path checks
`SetExistingFile`, `SetExistingDirectory`, `SetExistingPath`,
`SetNonexistentPath`.

## Mixing positional and named arguments

Positional and named arguments can be declared and passed in any order — the
parser assigns bare values to positionals left to right while pulling named
options out of the stream.

```cpp
auto parser = argparse::ArgumentParser("cp");
parser.AddArgument(argparse::CreatePositionalArgument("src"));
parser.AddArgument(argparse::CreatePositionalArgument("dst"));
parser.AddArgument(argparse::CreateNamedArgument('f', "force",
    0, argparse::ArgTypeCast::e_bool, false));   // a flag

// all equivalent:  a b --force  |  --force a b  |  a --force b
auto obj = parser.ParseArgs(std::vector<std::string>{ "a", "--force", "b" });
std::string src = obj.GetAsString("src");   // "a"
std::string dst = obj.GetAsString("dst");   // "b"
```

## Reading results by name

After a successful parse you can read values straight off the result by
argument name, without going through `GetArg(...)` first:

```cpp
int                       n     = obj.GetAsInt("count");
double                    ratio = obj.GetAsDouble("ratio");
std::string               name  = obj.GetAsString("name");
const std::vector<int>&   nums  = obj.GetAsVecInt("numbers");
```

The longer `obj.GetArg("count").GetAsInt()` form still works and is handy when
you want to inspect the `Argument` itself (e.g. `GetArgumentExists()`).

## Configuring the parser

`ArgumentParser` can be built from a `ParserSpec` aggregate, which collects the
parser-level options in one place (the counterpart of the keyword-style argument
specs):

```cpp
auto parser = argparse::ArgumentParser(argparse::ParserSpec{
    .name        = "cptool",
    .description = "Copy files",
    .prefixChars = '-',
    .addHelp     = true,
    .allowAbbrev = false});
```

The designated-initializer form needs C++20; the same struct also works with
ordinary aggregate initialization in C++11/14/17.

## Using it as a C++20 module

On toolchains that support C++20 modules (MSVC, GCC ≥ 14) you can consume the
library through `import` instead of `#include`:

```cpp
import argparse;

int main(int argc, char** argv)
{
    argparse::ArgumentParser parser("demo");
    // ... same API as the header ...
}
```

Add `argparse.ixx` to your build as a module interface unit; it wraps
`argparse.h` and re-exports the public API. The header remains fully usable on
its own, so nothing changes for `#include` users. Define
`ARGPARSE_NAMESPACE_NAME` when building the module to rename the exported
namespace, exactly as with the header.

## Requirements

- A **C++11** compiler or newer. The library is continuously tested on
  **C++17, C++20 and C++23** across Linux, macOS and Windows.
- The `std::any`-based `Get()` accessor is available from **C++17** onward; the
  typed getters (`GetAsInt()`, `GetAsVecInt()`, …) work in every standard.
- Designated-initializer (keyword) argument construction needs **C++20**.

## Building the tests

The tests live under `tests/` and are never included by the header, so consumers
can ignore them entirely.

```console
cmake -S tests -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Override the standard with `-DCMAKE_CXX_STANDARD=20` (or `23`).

## Documentation

- 📘 **[Tutorial](https://github.com/simfeo/FancyArgumentParser/wiki/Tutorial)** — step-by-step introduction.
- 📗 **[Examples](https://github.com/simfeo/FancyArgumentParser/wiki/Examples)** — flags, choices, positional/named mixes, bool, ranges, keyword args, and more.
- 📙 **[Class reference](https://github.com/simfeo/FancyArgumentParser/wiki/Classes-description)** — full API overview.
- 📓 **[About](https://github.com/simfeo/FancyArgumentParser/wiki/About-Fancy-Argument-Parser)** — project background.

## License

Released under the **MIT License** — see the header of
[`argparse.h`](argparse.h) for the full text. © 2021 simfeo.
