# Tests

These tests are for developing FancyArgumentParser itself. **They are not part
of the library.** If you only want to use the parser, copy `argparse.h` into
your project — nothing in this folder is included by the header, so it never
affects your build.

## Running

With CMake (from the repository root):

```sh
cmake -S tests -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or directly, without CMake:

```sh
c++ -std=c++17 -Wall -Wextra -I.. tests/test_argparse.cpp -o test_argparse
./test_argparse
```

The suite is a single, dependency-free `test_argparse.cpp` using a tiny
assertion framework. Add a `static void test_*()` function and register it with
`RUN(...)` in `main()` to add a case.
