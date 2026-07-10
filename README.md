<div align="center">

# FancyArgumentParser

**A single-header, dependency-free C++ command-line argument parser.**

Argparse-style ergonomics for C++ — named & positional arguments, type casting,
choices, defaults, auto-generated help, and Python-like keyword arguments in C++20.

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
- 🏷️ **Named and positional arguments**, freely mixed.
- 🔢 **Typed values** — `int`, `long long`, `double`, `bool`, and `string`.
- 🎚️ **Flexible arity** — fixed counts, `kAnyArgCount` (zero-or-more),
  `kFromOneToInfiniteArgCount` (one-or-more), or flags (zero values).
- ✅ **Validation** — required/optional, value `choices`, and defaults.
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
