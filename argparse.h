/**
MIT License

Copyright(c) 2021 simfeo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

// define your own macro ARGPARSE_NAMESPACE_NAME
// if you doesn't like default namespace "argparse"
#ifndef ARGPARSE_NAMESPACE_NAME
#define ARGPARSE_NAMESPACE_NAME argparse
#endif

#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <initializer_list>
#include <functional>
#include <regex>
#include <cctype>

#if __cplusplus > 201402L || _MSVC_LANG > 201402L
#include <any>
#endif

// std::filesystem is available from C++17. The path-existence validators
// (SetExistingFile / ...) are only compiled when it is present.
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#include <filesystem>
#define ARGPARSE_HAS_FILESYSTEM 1
#endif

/// @brief namespace of argument parser constants and Classes
/// can be changed if ARGPARSE_NAMESPACE_NAME macro specified during compilation.
/// By default is namespace name is "argparse"
namespace ARGPARSE_NAMESPACE_NAME
{
    /// @brief internal helpers. A named, inline namespace (rather than an
    /// anonymous one) so the symbols have external linkage and get emitted in
    /// module consumers; inline keeps #include across multiple TUs valid.
#ifdef __cpp_inline_variables
#define ARGPARSE_DETAIL_CONST inline constexpr
#else
#define ARGPARSE_DETAIL_CONST const
#endif
    namespace detail
    {
        ARGPARSE_DETAIL_CONST size_t kSizeTypeEnd = static_cast<size_t>(-1);
        ARGPARSE_DETAIL_CONST size_t kHelpWidth = 80;
        ARGPARSE_DETAIL_CONST size_t kHelpNameWidthPercent = 30;

        inline bool iEquals(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size())
            {
                return false;
            }
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i]))
                    != std::tolower(static_cast<unsigned char>(b[i])))
                {
                    return false;
                }
            }
            return true;
        }

        inline bool isNumber(const std::string& inStr)
        {
            const bool hasNegSign = inStr.at(0) == '-';
            size_t dotPos = 0, expPos = 0;
            size_t startPos = static_cast<size_t>(hasNegSign);
            for (size_t curPos = startPos; curPos < inStr.size(); ++curPos)
            {
                const char curChar = inStr.at(curPos);
                if (curChar >= '0' && curChar <= '9')
                {
                    continue;
                }
                else if (curChar == '.'
                    && !dotPos && curPos != startPos && curPos != inStr.size() - 1)
                {
                    dotPos = curPos;
                }
                else if ((curChar == 'e' || curChar == 'E')
                    && !expPos && curPos != startPos && curPos != inStr.size() - 1)
                {
                    expPos = curPos;
                    if (!dotPos)
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        inline size_t getStringStreamLength(std::stringstream& showDesc)
        {
            showDesc.seekp(0, std::ios::end);
            return showDesc.tellp();
        }
    }
    using namespace detail;
#undef ARGPARSE_DETAIL_CONST


    /// @brief Supported types for argument
    /// If needed type is not in this list, then just use e_String
    enum class ArgTypeCast : int
    {
        e_String,
        e_int,
        e_longlong,
        e_double,
        e_bool
    };

    // inline (external linkage) where available so the module wrapper can
    // export them; plain const (internal linkage) otherwise. Behaviour for
    // #include users is identical.
#ifdef __cpp_inline_variables
#define ARGPARSE_CONST inline constexpr
#else
#define ARGPARSE_CONST const
#endif
    /// @brief constant to indicate arguments with various
    /// count from 0 to infinite
    ARGPARSE_CONST int kAnyArgCount = -1;
    /// @brief constant to indicate arguments with various
    /// count from 1 to infinite
    ARGPARSE_CONST int kFromOneToInfiniteArgCount = -2;
    /// @brief constant to indicate an argument that takes zero or one value
    /// (Python's nargs='?').
    ARGPARSE_CONST int kZeroOrOneArgCount = -3;
#undef ARGPARSE_CONST

    /// @brief Argument count value. Accepts either an integer (an exact count,
    /// or one of the k...ArgCount constants) or a Python-style character:
    /// '?' (zero-or-one), '*' (zero-or-more), '+' (one-or-more).
    /// Implicitly convertible to int so it can be used anywhere a plain count is
    /// expected. An invalid character throws std::runtime_error at definition time.
    struct NArgs
    {
        int value;

        NArgs(int n = 1) : value(n) {}
        NArgs(char c) : value(FromChar(c)) {}

        operator int() const { return value; }

        static int FromChar(char c)
        {
            switch (c)
            {
            case '?': return kZeroOrOneArgCount;
            case '*': return kAnyArgCount;
            case '+': return kFromOneToInfiniteArgCount;
            default:
                throw std::runtime_error(
                    std::string("invalid nargs character '") + c
                    + "'; expected '?', '*' or '+'");
            }
        }
    };


    /// @brief Argument name value. Accepts a std::string, a string literal, or a
    /// single char -- so a short name can be written as 'f' as well as "f".
    /// Implicitly converts to std::string, so it works anywhere a name is taken.
    struct ArgName
    {
        std::string value;

        ArgName() {}
        ArgName(char c) : value(1, c) {}
        ArgName(const char* s) : value(s ? s : "") {}
        ArgName(const std::string& s) : value(s) {}

        operator const std::string&() const { return value; }
    };


    class ArgumentParser;
    class ArgumentsObject;
    class ArgumentParsed;

    /// @brief This class represents argument configuration
    /// which should be passed to ArgumentParser objects instance
    class Argument
    {
        /// @brief Default constructor. Private!
        /// @param positionalName represents name for positional argument.
        /// @param shortName represents short name for named argument which should be passed with one prefix.
        /// @param longName represents long name for named argument which should be passed with double prefix.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfiniteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        /// @param predicate Optional value validator: returns true for an accepted
        /// value token; nullptr (the default) installs no validator.
        /// @param validatorMessage Error text used when @p predicate rejects a value
        /// ("" uses a generated default message).
        Argument(const std::string& positionalName = "",
            const std::string& shortName = "",
            const std::string& longName = "",
            const int argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "",
            std::function<bool(const std::string&)> predicate = nullptr,
            const std::string& validatorMessage = ""
        )
            : m_required(required)
            , m_nargs(argsCount)
            , m_type(argType)
            , m_positionalName(positionalName)
            , m_shortName(shortName)
            , m_longName(longName)
            , m_help(help)
            , m_validator(predicate)
            , m_validatorMessage(validatorMessage)
        {}
    public:

        /// @brief Default constructor positional arguments. You can use class Setters or pass your own values to public members directly.
        /// @param shortName represents short name for named argument which should be passed with one prefix.
        /// @param longName represents long name for named argument which should be passed with double prefix.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfiniteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        /// @param predicate Optional value validator: returns true for an accepted
        /// value token; nullptr (the default) installs no validator.
        /// @param validatorMessage Error text used when @p predicate rejects a value
        /// ("" uses a generated default message).
        static Argument CreateNamedArgument(const ArgName& shortName = "",
            const ArgName& longName = "",
            NArgs argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "",
            std::function<bool(const std::string&)> predicate = nullptr,
            const std::string& validatorMessage = "")
        {
            return Argument("", shortName, longName, argsCount, argType, required, help, predicate, validatorMessage);
        }

        /// @brief Default function for named arguments. You can use class Setters or pass your own values to public members directly.
        /// @param positionalName represents name for positional argument.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfiniteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        /// @param predicate Optional value validator: returns true for an accepted
        /// value token; nullptr (the default) installs no validator.
        /// @param validatorMessage Error text used when @p predicate rejects a value
        /// ("" uses a generated default message).
        static Argument CreatePositionalArgument(const ArgName& positionalName = "",
            NArgs argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "",
            std::function<bool(const std::string&)> predicate = nullptr,
            const std::string& validatorMessage = "")
        {
            return Argument(positionalName, "", "", argsCount, argType, required, help, predicate, validatorMessage);
        }

        /// @brief required flag argument
        /// If required argument is not set in command line
        /// then parsing will fail.
        ///
        /// IMPORTANT: ALL arguments -- named AND positional -- are REQUIRED by
        /// default. This differs from Python's argparse, where named options are
        /// optional by default. To make an argument optional call
        /// SetRequired(false) (or pass required=false to the factory function).
        /// Optional flags in particular almost always want SetRequired(false).
        bool m_required = true;

        /// @brief Setter function to flag,
        /// @param required - bool value to indicate is argument required or not
        /// @return reference to current argument
        Argument& SetRequired(bool required)
        {
            m_required = required;
            return *this;
        }

        /// @brief variable that indicates count of argument in input
        /// use "kAnyArgCount" or "kFromOneToInfiniteArgCount" constants
        /// for arguments with variable count. Any other arguments count
        /// will be passed as strict arguments count.
        /// 0 is for flags (arguments that doesn't carry any data)
        /// set to 1 by default.
        int m_nargs = 1;

        /// @brief setter function for m_nargs with desired amount
        /// @param amount argument count: an integer (exact count or a
        /// k...ArgCount constant), or a Python-style character '?' / '*' / '+'.
        /// @return reference to current argument
        Argument& SetNumberOfArguments(NArgs amount)
        {
            m_nargs = amount;
            return *this;
        }

        /// @brief Handy setter for an argument that takes zero or one value
        /// (Python's nargs='?').
        /// @return reference to current argument
        Argument& SetZeroOrOneArgument()
        {
            m_nargs = kZeroOrOneArgCount;
            return *this;
        }

        /// @brief Handy setter for argument count with self declared name
        /// @return reference to current argument
        Argument& SetAnyNumberOfArgumentsButAtLeastOne()
        {
            m_nargs = kFromOneToInfiniteArgCount;
            return *this;
        }

        /// @brief Handy setter for argument count with self declared name
        /// @return reference to current argument
        Argument& SetAnyNumberOfArguments()
        {
            m_nargs = kAnyArgCount;
            return *this;
        }

        /// @brief Handy setter for argument which is actually a flag (e.g. has no any parameters)
        /// @return reference to current argument
        Argument& SetArgumentIsFlag()
        {
            m_nargs = 0;
            return *this;
        }

        /// @brief Variable that hold type of argument.
        /// string by default
        ArgTypeCast m_type = ArgTypeCast::e_String;

        /// @brief Setter function for type of current argument
        /// @param argType setter for type of current argument data.
        /// Any non string types will be casted while parsing.
        /// @return reference to current argument
        Argument& SetType(ArgTypeCast argType)
        {
            m_type = argType;
            return *this;
        }

        /// @brief name of positional argument
        /// positional arguments name used only to access desired argument from code
        std::string m_positionalName = "";

        /// @brief Handy setter for positional argument
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetPositionalName(const ArgName& name)
        {
            m_positionalName = name;
            return *this;
        }

        /// @brief arguments short name
        /// for non positional argument only
        /// shot name used in input with 1 prefix
        std::string m_shortName = "";

        /// @brief Handy setter for short named argument. 
        /// Should been used with ordinary prefix in command line.
        /// Can be auto-generated if possible when m_allowAbbrev in ArgumentParsed set to true.
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetShortName(const ArgName& name)
        {
            m_shortName = name;
            return *this;
        }
        
        /// @brief arguments long name.
        /// for non positional arguments only
        /// argument long name start with double prefix
        std::string m_longName = "";

        /// @brief Handy setter for long named argument. 
        /// Should been used with double prefix in command line.
        /// Can be used for auto-generation of short name if it possible
        /// and m_allowAbbrev in ArgumentParsed is "true".
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetLongName(const ArgName& name)
        {
            m_longName = name;
            return *this;
        }

        /// @brief additional help info for argument
        /// Will be part of generated help
        std::string m_help = "";

        /// @brief Handy setter for additional help
        /// @param help string with additional help. Empty by default.
        /// @return reference to current argument
        Argument& SetHelp(const std::string& help)
        {
            m_help = help;
            return *this;
        }
        
        /// @brief vector of strings to validate arguments input data.
        /// Empty by default. Will fail parsing if string not is in input list
        std::vector<std::string> m_choicesString = {};
        /// @brief when true, string choices are matched case-insensitively
        bool m_choicesIgnoreCase = false;

        /// @brief Handy setter of valid choices for arguments with string type
        /// @param choices vector or initializer list of valid strings
        /// @param ignoreCase match case-insensitively (false by default)
        /// @return reference to current argument
        Argument& SetChoices(const std::vector<std::string>& choices, bool ignoreCase = false)
        {
            if (m_type != ArgTypeCast::e_String)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesString = choices;
            m_choicesIgnoreCase = ignoreCase;
            return *this;
        }

        /// @brief Overload so a braced list of string literals -- e.g.
        /// SetChoices({"+", "-"}) -- resolves unambiguously to the string
        /// choices instead of colliding with the int/double/long long overloads.
        /// @param choices initializer list of string literals
        /// @param ignoreCase match case-insensitively (false by default)
        /// @return reference to current argument
        Argument& SetChoices(std::initializer_list<const char*> choices, bool ignoreCase = false)
        {
            return SetChoices(std::vector<std::string>(choices.begin(), choices.end()), ignoreCase);
        }

        /// @brief vector of integers to validate arguments input data.
        /// Empty by default. Will fail parsing if ints not is in input list
        std::vector<int> m_choicesInt = {};

        /// @brief Handy setter of valid choices for arguments with int type
        /// @param choices vector or initializer list of valid ints
        /// @return reference to current argument
        Argument& SetChoices(const std::vector<int>& choices)
        {
            if (m_type != ArgTypeCast::e_int)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesInt = choices;
            return *this;
        }

        /// @brief vector of long longs to validate arguments input data.
        /// Empty by default. Will fail parsing if long longs not is in input list
        std::vector<long long> m_choicesLongLong = {};

        /// @brief Handy setter of valid choices for arguments with long long type
        /// @param choices vector or initializer list of valid long longs
        /// @return reference to current argument
        Argument& SetChoices(const std::vector<long long>& choices)
        {
            if (m_type != ArgTypeCast::e_longlong)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesLongLong = choices;
            return *this;
        }

        /// @brief vector of double to validate arguments input data.
        /// Empty by default. Will fail parsing if double not is in input list
        std::vector<double> m_choicesDouble = {};

        /// @brief Handy setter of valid choices for arguments with double type
        /// @param choices vector or initializer list of valid double
        /// @return reference to current argument
        Argument& SetChoices(const std::vector<double>& choices)
        {
            if (m_type != ArgTypeCast::e_double)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesDouble = choices;
            return *this;
        }

        /// @brief Handy setter for single default argument of bool type
        /// @param defaultArg default boolean value
        /// @return reference to current argument
        Argument& SetDefault(bool defaultArg)
        {
            if (m_type != ArgTypeCast::e_bool)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultBool.push_back(defaultArg);
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for single default argument of int type
        /// @param defaultArg default int value
        /// @return reference to current argument
        Argument& SetDefault(int defaultArg)
        {
            if (m_type != ArgTypeCast::e_int)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultInt.push_back(defaultArg);
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for single default argument of long long type
        /// @param defaultArg default long long value
        /// @return reference to current argument
        Argument& SetDefault(long long defaultArg)
        {
            if (m_type != ArgTypeCast::e_longlong)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultLongLong.push_back(defaultArg);
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for single default argument of double type
        /// @param defaultArg default double value
        /// @return reference to current argument
        Argument& SetDefault(double defaultArg)
        {
            if (m_type != ArgTypeCast::e_double)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultDouble.push_back(defaultArg);
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for single default argument of string type
        /// @param defaultArg default string value
        /// @return reference to current argument
        Argument& SetDefault(std::string defaultArg)
        {
            if (m_type != ArgTypeCast::e_String)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultString.push_back(defaultArg);
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for vector of default arguments of bool type
        /// @param defaultArg vector of default boolean values
        /// @return reference to current argument
        Argument& SetDefault(const std::vector<bool>& defaultArg)
        {
            if (m_type != ArgTypeCast::e_bool)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultBool = defaultArg;
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for vector of default arguments of int type
        /// @param defaultArg vector of default int values
        /// @return reference to current argument
        Argument& SetDefault(const std::vector<int>& defaultArg)
        {
            if (m_type != ArgTypeCast::e_int)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultInt = defaultArg;
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for vector of default arguments of long long type
        /// @param defaultArg vector of default long values
        /// @return reference to current argument
        Argument& SetDefault(const std::vector<long long>& defaultArg)
        {
            if (m_type != ArgTypeCast::e_longlong)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultLongLong = defaultArg;
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for vector of default arguments of double type
        /// @param defaultArg vector of default double values
        /// @return reference to current argument
        Argument& SetDefault(const std::vector<double>& defaultArg)
        {
            if (m_type != ArgTypeCast::e_double)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultDouble = defaultArg;
            m_hasDefault = true;
            return *this;
        }

        /// @brief Handy setter for vector of default arguments of string type
        /// @param defaultArg vector of default string values
        /// @return reference to current argument
        Argument& SetDefault(const std::vector<std::string>& defaultArg)
        {
            if (m_type != ArgTypeCast::e_String)
            {
                throw std::runtime_error("wrong type");
            }
            m_defaultString = defaultArg;
            m_hasDefault = true;
            return *this;
        }


        /// @brief Getter to indicate does argument has any default value
        /// @return reference to current argument
        bool HasDefault() const
        {
            return m_hasDefault;
        }

        /// @brief Bind a variable to this argument. After a successful
        /// ParseArgs(), the parsed value is written directly into *target, so
        /// you no longer have to pull it out with GetArg(name).GetAsX().
        ///
        /// BindTo also sets this argument's type to match the bound variable,
        /// so a separate SetType() call is not needed (and should not be used
        /// to contradict it).
        ///
        /// IMPORTANT: *target must outlive the ParseArgs() call. If the
        /// argument is optional and absent (with no default), the bound
        /// variable is left untouched -- initialize it yourself for a default.
        /// @param target pointer to the variable that receives the parsed value
        /// @return reference to current argument
        Argument& BindTo(bool* target);
        Argument& BindTo(int* target);
        Argument& BindTo(long long* target);
        Argument& BindTo(double* target);
        Argument& BindTo(std::string* target);
        Argument& BindTo(std::vector<bool>* target);
        Argument& BindTo(std::vector<int>* target);
        Argument& BindTo(std::vector<long long>* target);
        Argument& BindTo(std::vector<double>* target);
        Argument& BindTo(std::vector<std::string>* target);

        /// @brief Does this argument have a bound variable (see BindTo)
        /// @return true if BindTo(...) was called on this argument
        bool HasBinding() const
        {
            return static_cast<bool>(m_binding);
        }

        /// @brief Apply the binding (if any) from a parsed result. Called by
        /// ArgumentParser after a successful parse; a no-op when unbound.
        /// @param parsed the parsed values for this argument
        void ApplyBinding(const ArgumentParsed& parsed) const
        {
            if (m_binding)
            {
                m_binding(parsed);
            }
        }

        /// @brief Install a validator: each parsed value token must satisfy
        /// @p predicate, otherwise parsing fails. Runs on the raw value (so it
        /// works for any type; convert inside the predicate if needed).
        /// @param predicate returns true for an accepted value
        /// @param message custom error text (a default is used when empty)
        /// @return reference to current argument
        Argument& SetValidator(std::function<bool(const std::string&)> predicate,
            const std::string& message = "")
        {
            m_validator = std::move(predicate);
            m_validatorMessage = message;
            return *this;
        }

        /// @brief Whether a value passes this argument's validator (true if none).
        bool RunValidator(const std::string& value) const
        {
            return !m_validator || m_validator(value);
        }

        /// @brief Custom validator error message ("" means use the default).
        const std::string& ValidatorMessage() const
        {
            return m_validatorMessage;
        }

        /// @brief Restrict an integer argument to the inclusive range [lo, hi].
        /// Sets the type to e_int and installs a validator.
        Argument& SetRange(int lo, int hi)
        {
            m_type = ArgTypeCast::e_int;
            return SetRangeLL(lo, hi);
        }

        /// @brief Restrict a long long argument to the inclusive range [lo, hi].
        Argument& SetRange(long long lo, long long hi)
        {
            m_type = ArgTypeCast::e_longlong;
            return SetRangeLL(lo, hi);
        }

        /// @brief Restrict a double argument to the inclusive range [lo, hi].
        Argument& SetRange(double lo, double hi)
        {
            m_type = ArgTypeCast::e_double;
            return SetValidator(
                [lo, hi](const std::string& s) {
                    try { double v = std::stod(s); return v >= lo && v <= hi; }
                    catch (...) { return false; }
                },
                "value out of range [" + std::to_string(lo) + ", " + std::to_string(hi) + "]");
        }

        /// @brief Restrict an integer argument to [0, max].
        Argument& SetRange(int max) { return SetRange(0, max); }
        /// @brief Restrict a long long argument to [0, max].
        Argument& SetRange(long long max) { return SetRange(0LL, max); }
        /// @brief Restrict a double argument to [0, max].
        Argument& SetRange(double max) { return SetRange(0.0, max); }

        /// @brief Require a strictly positive number (> 0). Type-agnostic.
        Argument& SetPositive(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    try { return std::stod(s) > 0.0; } catch (...) { return false; }
                },
                message.empty() ? "value must be positive" : message);
        }

        /// @brief Require a non-negative number (>= 0). Type-agnostic.
        Argument& SetNonNegative(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    try { return std::stod(s) >= 0.0; } catch (...) { return false; }
                },
                message.empty() ? "value must be non-negative" : message);
        }

        /// @brief Require the value to fully match an ECMAScript regular
        /// expression. An invalid pattern throws std::regex_error at definition.
        Argument& SetPattern(const std::string& pattern, const std::string& message = "")
        {
            std::regex re(pattern);
            return SetValidator(
                [re](const std::string& s) { return std::regex_match(s, re); },
                message.empty() ? ("value does not match pattern \"" + pattern + "\"") : message);
        }

#ifdef ARGPARSE_HAS_FILESYSTEM
        /// @brief Require the value to name an existing regular file (C++17+).
        Argument& SetExistingFile(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    std::error_code ec; return std::filesystem::is_regular_file(s, ec);
                },
                message.empty() ? "file does not exist" : message);
        }

        /// @brief Require the value to name an existing directory (C++17+).
        Argument& SetExistingDirectory(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    std::error_code ec; return std::filesystem::is_directory(s, ec);
                },
                message.empty() ? "directory does not exist" : message);
        }

        /// @brief Require the value to name an existing path (C++17+).
        Argument& SetExistingPath(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    std::error_code ec; return std::filesystem::exists(s, ec);
                },
                message.empty() ? "path does not exist" : message);
        }

        /// @brief Require the value to name a path that does NOT exist (C++17+).
        Argument& SetNonexistentPath(const std::string& message = "")
        {
            return SetValidator(
                [](const std::string& s) {
                    std::error_code ec; return !std::filesystem::exists(s, ec);
                },
                message.empty() ? "path already exists" : message);
        }
#endif

    private:
        /// @brief Shared integer-range validator for SetRange(int) / SetRange(long long).
        Argument& SetRangeLL(long long lo, long long hi)
        {
            return SetValidator(
                [lo, hi](const std::string& s) {
                    try { long long v = std::stoll(s); return v >= lo && v <= hi; }
                    catch (...) { return false; }
                },
                "value out of range [" + std::to_string(lo) + ", " + std::to_string(hi) + "]");
        }

        /// @brief type-erased sink installed by BindTo(...); empty when unbound
        std::function<void(const ArgumentParsed&)> m_binding = nullptr;
        /// @brief value predicate installed by SetValidator(...); empty when unset
        std::function<bool(const std::string&)> m_validator = nullptr;
        std::string m_validatorMessage = "";

        bool                     m_hasDefault = false;
        std::vector<bool>        m_defaultBool = {};
        std::vector<int>         m_defaultInt = {};
        std::vector<long long>   m_defaultLongLong = {};
        std::vector<double>      m_defaultDouble = {};
        std::vector<std::string> m_defaultString = {};

        friend ArgumentsObject;
    };

    /// @brief Helper function to create named argument
    /// @param shortName Short name if needed. Will be used with single prefix.
    /// Short name could be auto-generated if possible when m_allowAbbrev in ArgumentParsed set to true)
    /// @param longName Full name of argument
    /// @param argsCount Count of arguments
    /// (use kFromOneToInfiniteArgCount or kAnyArgCount for various arguments count)
    /// @param argType e_String, e_int, e_longlong, e_double, e_bool
    /// @param required Marker if argument should be passed or ignored if missed.
    /// @param help Initial part of help for current argument in case of auto-generated help.
    /// @param predicate Optional value validator: returns true for an accepted
    /// value token; nullptr (the default) installs no validator.
    /// @param validatorMessage Error text used when @p predicate rejects a value
    /// ("" uses a generated default message).
    /// @return instance of Argument
    /// @note inline: this is a free function in a header, so it must have
    /// inline linkage to be safely included in more than one translation unit.
    inline Argument CreateNamedArgument(const ArgName& shortName = "",
        const ArgName& longName = "",
        NArgs argsCount = 1,
        ArgTypeCast argType = ArgTypeCast::e_String,
        const bool required = true,
        const std::string& help = "",
        std::function<bool(const std::string&)> predicate = nullptr,
        const std::string& validatorMessage = "")
    {
        return Argument::CreateNamedArgument(shortName, longName, argsCount, argType, required, help, predicate, validatorMessage);
    }

    /// @brief Helper function to create positional argument
    /// @param positionalName Name of positional argument to access from code.
    /// @param argsCount Count of arguments
    /// (use kFromOneToInfiniteArgCount or kAnyArgCount for various arguments count)
    /// @param argType e_String, e_int, e_longlong, e_double, e_bool
    /// @param required Marker if argument should be passed or ignored if missed.
    /// @param help Initial part of help for current argument in case of auto-generated help.
    /// @param predicate Optional value validator: returns true for an accepted
    /// value token; nullptr (the default) installs no validator.
    /// @param validatorMessage Error text used when @p predicate rejects a value
    /// ("" uses a generated default message).
    /// @return instance of Argument
    /// @note inline: see CreateNamedArgument -- required for multi-TU inclusion.
    inline Argument CreatePositionalArgument(const ArgName& positionalName = "",
        NArgs argsCount = 1,
        ArgTypeCast argType = ArgTypeCast::e_String,
        const bool required = true,
        const std::string& help = "",
        std::function<bool(const std::string&)> predicate = nullptr,
        const std::string& validatorMessage = "")
    {
        return Argument::CreatePositionalArgument(positionalName, argsCount, argType, required, help, predicate, validatorMessage);
    }


    /// @brief Aggregate description of a named argument, for keyword-style
    /// construction. Because it is a plain aggregate, C++20 designated
    /// initializers give a Python-like call site:
    /// @code
    ///   parser.AddArgument(argparse::CreateNamedArgument({
    ///       .longName = "numbers",
    ///       .nargs    = argparse::kFromOneToInfiniteArgCount,
    ///       .type     = argparse::ArgTypeCast::e_int,
    ///       .required = false,
    ///       .help     = "some numbers",
    ///       .validator = [](const std::string&)->bool{ return false; },
    ///       .validator_message = "wrong input for numbers"}));
    /// @endcode
    /// The same struct also works with ordinary aggregate init in C++11/14/17.
    struct NamedArgSpec
    {
        ArgName shortName = "";
        ArgName longName = "";
        NArgs nargs = 1;
        ArgTypeCast type = ArgTypeCast::e_String;
        bool required = true;
        std::string help = "";
        std::function<bool(const std::string&)> validator = nullptr;
        std::string validator_message = "";
    };

    /// @brief Keyword-style factory for a named argument. See NamedArgSpec.
    /// @param spec aggregate of the argument's properties
    /// @return instance of Argument
    inline Argument CreateNamedArgument(const NamedArgSpec& spec)
    {
        return Argument::CreateNamedArgument(spec.shortName, spec.longName,
            spec.nargs, spec.type, spec.required, spec.help, spec.validator, spec.validator_message);
    }

    /// @brief Aggregate description of a positional argument, for keyword-style
    /// construction with C++20 designated initializers. See NamedArgSpec.
    struct PositionalArgSpec
    {
        std::string name = "";
        NArgs nargs = 1;
        ArgTypeCast type = ArgTypeCast::e_String;
        bool required = true;
        std::string help = "";
        std::function<bool(const std::string&)> validator = nullptr;
        std::string validator_message = "";
    };

    /// @brief Keyword-style factory for a positional argument. See PositionalArgSpec.
    /// @param spec aggregate of the argument's properties
    /// @return instance of Argument
    inline Argument CreatePositionalArgument(const PositionalArgSpec& spec)
    {
        return Argument::CreatePositionalArgument(spec.name, spec.nargs,
            spec.type, spec.required, spec.help, spec.validator, spec.validator_message);
    }

    /// @brief Class which represent actual parsed argument in case of successfully parsing
    class ArgumentParsed
    {
    public:
        /// @brief Does argument exists. Needed to check in case when argument is not required
        /// @return bool value
        bool GetArgumentExists()
        {
            return m_exists;
        }

        /// @brief get actual count of argument, in case of various arguments count.
        /// @return 
        size_t GetArgumentCount()
        {
            return m_count;
        }

        /// @brief Get result as single bool for bool type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return bool value of argument
        /// @throws std::out_of_range if the argument holds no value
        bool GetAsBool() const
        {
            ThrowIfEmpty(m_bool.empty(), "GetAsBool");
            return m_bool.front();
        }

        /// @brief Get result as single int for int type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return int value of argument
        /// @throws std::out_of_range if the argument holds no value
        int GetAsInt() const
        {
            ThrowIfEmpty(m_int.empty(), "GetAsInt");
            return m_int.front();
        }

        /// @brief Get result as single long long for long long type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return long long value of argument
        /// @throws std::out_of_range if the argument holds no value
        long long GetAsLongLong() const
        {
            ThrowIfEmpty(m_longLong.empty(), "GetAsLongLong");
            return m_longLong.front();
        }


        /// @brief Get result as single double for double type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return double value of argument
        /// @throws std::out_of_range if the argument holds no value
        double GetAsDouble() const
        {
            ThrowIfEmpty(m_double.empty(), "GetAsDouble");
            return m_double.front();
        }


        /// @brief Get result as single string for string type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return string value of argument (returned by value so it never dangles)
        /// @throws std::out_of_range if the argument holds no value
        std::string GetAsString() const
        {
            ThrowIfEmpty(m_string.empty(), "GetAsString");
            return m_string.front();
        }

        /// @brief Get result as vector bool for bool type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector bool value of argument (by value: safe to store even when
        /// called on a temporary ArgumentParsed returned by GetArg())
        std::vector<bool> GetAsVecBool() const
        {
            return m_bool;
        }

        /// @brief Get result as vector int for int type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector int value of argument (by value, see GetAsVecBool)
        std::vector<int> GetAsVecInt() const
        {
            return m_int;
        }

        /// @brief Get result as vector long long for long long type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector long long value of argument (by value, see GetAsVecBool)
        std::vector<long long> GetAsVecLongLong() const
        {
            return m_longLong;
        }

        /// @brief Get result as vector double for double type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector double value of argument (by value, see GetAsVecBool)
        std::vector<double> GetAsVecDouble() const
        {
            return m_double;
        }

        /// @brief Get result as vector string for string type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector string value of argument (by value, see GetAsVecBool)
        std::vector<std::string> GetAsVecString() const
        {
            return m_string;
        }

#if __cplusplus > 201402L || _MSVC_LANG > 201402L
        /// @brief Function to get actual argument value
        /// @return std::any: could be bool, int, long long, double, string 
        /// and vector variants of same types regarding of arguments type.
        std::any Get()
        {
            switch (m_type)
            {
            case ArgTypeCast::e_String:
                if (m_count == 1)
                {
                    return m_string.front();
                }
                return m_string;
                break;
            case ArgTypeCast::e_int:
                if (m_count == 1)
                {
                    return m_int.front();
                }
                return m_int;
                break;
            case ArgTypeCast::e_longlong:
                if (m_count == 1)
                {
                    return m_longLong.front();
                }
                return m_longLong;
                break;
            case ArgTypeCast::e_double:
                if (m_count == 1)
                {
                    return m_double.front();
                }
                return m_double;
                break;
            case ArgTypeCast::e_bool:
            default:
                if (m_count == 1)
                {
                    return m_bool.front();
                }
                return m_bool;
                break;
            }
        }

#endif // __cplusplus >= 


    protected:

        ArgumentParsed() {}

        /// @brief Guard for the scalar getters. Turns an out-of-bounds front()
        /// (undefined behavior) into a clear, catchable exception. Call
        /// GetArgumentExists()/GetArgumentCount() first to avoid it.
        static void ThrowIfEmpty(bool empty, const char* getter)
        {
            if (empty)
            {
                throw std::out_of_range(std::string("ArgumentParsed::") + getter
                    + "() called on an argument that holds no value");
            }
        }

        /// @brief flag about is argument exists
        bool        m_exists{ false };
        /// @brief type of argument
        ArgTypeCast m_type{ ArgTypeCast::e_String };
        /// @brief count of arguments properties
        size_t      m_count{ 0 };

        /// @brief container of parsed bool arguments
        std::vector<bool>        m_bool = {};
        /// @brief container of parse int arguments
        std::vector<int>         m_int = {};
        /// @brief container of parsed long long arguments
        std::vector<long long>   m_longLong = {};
        /// @brief container of parsed double arguments
        std::vector<double>      m_double = {};
        /// @brief container of parsed string arguments
        std::vector<std::string> m_string = {};

        friend ArgumentsObject;
    };

    // Out-of-line: BindTo needs ArgumentParsed's getters, complete only here.
    // Each overload also sets the argument type to match the bound variable.

    inline Argument& Argument::BindTo(bool* target)
    {
        m_type = ArgTypeCast::e_bool;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsBool(); };
        return *this;
    }
    inline Argument& Argument::BindTo(int* target)
    {
        m_type = ArgTypeCast::e_int;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsInt(); };
        return *this;
    }
    inline Argument& Argument::BindTo(long long* target)
    {
        m_type = ArgTypeCast::e_longlong;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsLongLong(); };
        return *this;
    }
    inline Argument& Argument::BindTo(double* target)
    {
        m_type = ArgTypeCast::e_double;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsDouble(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::string* target)
    {
        m_type = ArgTypeCast::e_String;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsString(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::vector<bool>* target)
    {
        m_type = ArgTypeCast::e_bool;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsVecBool(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::vector<int>* target)
    {
        m_type = ArgTypeCast::e_int;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsVecInt(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::vector<long long>* target)
    {
        m_type = ArgTypeCast::e_longlong;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsVecLongLong(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::vector<double>* target)
    {
        m_type = ArgTypeCast::e_double;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsVecDouble(); };
        return *this;
    }
    inline Argument& Argument::BindTo(std::vector<std::string>* target)
    {
        m_type = ArgTypeCast::e_String;
        m_binding = [target](const ArgumentParsed& parsed) { *target = parsed.GetAsVecString(); };
        return *this;
    }

    /// @brief Class that carries result of real parsing
    /// Indicates if parsing is successful and allows to get ArgumentParsed object in that case.
    /// In case of parsing failure provides error string for first error.
    class ArgumentsObject
    {
    public:
        /// @brief Indicates if parsing was successful
        /// @return bool value
        bool IsArgValid() const
        {
            return m_isValid;
        }

        /// @brief Function to get error message in case of parsing failure.
        /// @return string with error message
        const std::string& GetErrorString()
        {
            return m_error;
        }

        /// @brief I don't know when you could need this info
        /// @return size_t count of successfully parsed arguments
        size_t ParsedArgsCount() const
        {
            return m_parsed.size();
        }

        /// @brief Getter function to get ArgumentParsed object 
        /// @param name name of argument object, could be short name, long name or positional name
        /// @return Empty argument if argument with given name does not exist or real result if exists.
        ArgumentParsed GetArg(const std::string& name)
        {
            ArgumentParsed arg = ArgumentParsed();
            arg.m_exists = false;
            arg.m_count = 0;

            const auto position = m_names.find(name);
            if (position == m_names.end())
            {
                return arg;
            }

            const auto argument = m_parsed.find(position->second);
            if (argument == m_parsed.end())
            {
                return arg;
            }

            return argument->second;
        }

        /// @name By-name value shortcuts
        /// Equivalent to GetArg(name).GetAsX(). The scalar forms throw
        /// std::out_of_range when the argument holds no value, so guard optional
        /// arguments with GetArg(name).GetArgumentExists() first.
        /// @{
        bool        GetAsBool(const std::string& name)   { return GetArg(name).GetAsBool(); }
        int         GetAsInt(const std::string& name)    { return GetArg(name).GetAsInt(); }
        long long   GetAsLongLong(const std::string& name) { return GetArg(name).GetAsLongLong(); }
        double      GetAsDouble(const std::string& name) { return GetArg(name).GetAsDouble(); }
        std::string GetAsString(const std::string& name) { return GetArg(name).GetAsString(); }

        std::vector<bool>        GetAsVecBool(const std::string& name)     { return GetArg(name).GetAsVecBool(); }
        std::vector<int>         GetAsVecInt(const std::string& name)      { return GetArg(name).GetAsVecInt(); }
        std::vector<long long>   GetAsVecLongLong(const std::string& name) { return GetArg(name).GetAsVecLongLong(); }
        std::vector<double>      GetAsVecDouble(const std::string& name)   { return GetArg(name).GetAsVecDouble(); }
        std::vector<std::string> GetAsVecString(const std::string& name)   { return GetArg(name).GetAsVecString(); }
        /// @}

    private:
        ArgumentsObject() {}

        void SetValid()
        {
            m_isValid = true;
        }

        void SetErrorString(const std::string& error)
        {
            m_parsed.clear();
            m_error = error;
        }

        /// @brief Internal function to fill argument if default is argument absent
        /// @param argObj argument object in ArgumentParser::m_arguments
        /// @param position index of argument in ArgumentParser::m_arguments
        void ParseDefault(const Argument& argObj, const size_t position)
        {
            ArgumentParsed arg = ArgumentParsed();
            arg.m_exists = true;
            arg.m_type = argObj.m_type;

            if (argObj.m_type == ArgTypeCast::e_String)
            {
                arg.m_string = argObj.m_defaultString;
                arg.m_count = argObj.m_defaultString.size();
            }
            else if (argObj.m_type == ArgTypeCast::e_bool)
            {
                arg.m_bool = argObj.m_defaultBool;
                arg.m_count = argObj.m_defaultBool.size();
            }
            else if (argObj.m_type == ArgTypeCast::e_double)
            {
                arg.m_double = argObj.m_defaultDouble;
                arg.m_count = argObj.m_defaultDouble.size();
            }
            else if (argObj.m_type == ArgTypeCast::e_longlong)
            {
                arg.m_longLong = argObj.m_defaultLongLong;
                arg.m_count = argObj.m_defaultLongLong.size();
            }
            else if (argObj.m_type == ArgTypeCast::e_int)
            {
                arg.m_int = argObj.m_defaultInt;
                arg.m_count = argObj.m_defaultInt.size();
            }

            m_parsed.emplace(position, arg);
            if (!argObj.m_shortName.empty())
            {
                m_names[argObj.m_shortName] = position;
            }
            if (!argObj.m_longName.empty())
            {
                m_names[argObj.m_longName] = position;
            }
            if (!argObj.m_positionalName.empty())
            {
                m_names[argObj.m_positionalName] = position;
            }
        }

        /// @brief Function which creates holder for parsing various length arguments
        /// @param argObj argument object from ArgumentParser::m_arguments
        /// @param position index of argument in ArgumentParser::m_arguments
        /// @return true if the operation succeeded; false otherwise
        void CreateParsingStub(const Argument& argObj, const size_t position)
        {
            const auto argument = m_parsed.find(position);
            if (argument == m_parsed.end())
            {
                ArgumentParsed arg = ArgumentParsed();
                arg.m_exists = true;
                arg.m_count = 0;
                arg.m_type = argObj.m_type;
                m_parsed.emplace(position, arg);
                if (!argObj.m_shortName.empty())
                {
                    m_names[argObj.m_shortName] = position;
                }
                if (!argObj.m_longName.empty())
                {
                    m_names[argObj.m_longName] = position;
                }
                if (!argObj.m_positionalName.empty())
                {
                    m_names[argObj.m_positionalName] = position;
                }
            }
        }

        /// @brief Actual function which parses argument from command line
        /// @param argObj argument object from ArgumentParser::m_arguments
        /// @param position index of argument in ArgumentParser::m_arguments
        /// @param token token from command line input
        /// @return true if the operation succeeded; false otherwise
        bool Parse(const Argument& argObj, const size_t position, const std::string& token)
        {
            const auto argument = m_parsed.find(position);
            if (argument == m_parsed.end())
            {
                ArgumentParsed arg = ArgumentParsed();
                arg.m_exists = true;
                arg.m_count = 0;
                arg.m_type = argObj.m_type;
                m_parsed.emplace(position, arg);
                if (!argObj.m_shortName.empty())
                {
                    m_names[argObj.m_shortName] = position;
                }
                if (!argObj.m_longName.empty())
                {
                    m_names[argObj.m_longName] = position;
                }
                if (!argObj.m_positionalName.empty())
                {
                    m_names[argObj.m_positionalName] = position;
                }
                return true;
            }

            if (argObj.m_nargs != 0 && !argObj.RunValidator(token))
            {
                return InvalidateArgsValidator(argObj, token);
            }

            if (argument->second.m_type == ArgTypeCast::e_String)
            {
                if (argObj.m_nargs != 0)
                {
                    if (argObj.m_choicesString.size())
                    {
                        const bool ic = argObj.m_choicesIgnoreCase;
                        auto it = std::find_if(argObj.m_choicesString.begin(), argObj.m_choicesString.end(),
                            [&token, ic](const std::string& str) -> bool
                            {
                                return ic ? iEquals(token, str) : token == str;
                            });
                        if (it == argObj.m_choicesString.end())
                        {
                            return InvalidateArgsOutOfChoice(argObj, token);
                        }
                    }
                    argument->second.m_string.push_back(token);
                }
                else
                {
                    return InvalidateArgsTooMany(argObj);
                }
            }
            else if (argument->second.m_type == ArgTypeCast::e_bool)
            {
                if (argObj.m_nargs != 0)
                {
                    if (token == "True" || token == "TRUE" || token == "true")
                    {
                        argument->second.m_bool.push_back(true);
                    }
                    else if (token == "False" || token == "FALSE" || token == "false")
                    {
                        argument->second.m_bool.push_back(false);
                    }
                    else
                    {
                        return InvalidateArgsCannotParse(argObj, token);
                    }
                }
                else
                {
                    return InvalidateArgsTooMany(argObj);
                }
            }
            else if (argument->second.m_type == ArgTypeCast::e_int)
            {
                if (argObj.m_nargs != 0)
                {
                    if (isNumber(token))
                    {
                        try
                        {
                            argument->second.m_int.push_back(std::stoi(token));

                            int value = argument->second.m_int.back();

                            if (argObj.m_choicesInt.size())
                            {
                                auto it = std::find_if(argObj.m_choicesInt.begin(), argObj.m_choicesInt.end(),
                                    [value](const int& integerValue) -> bool
                                    {
                                        return value == integerValue;
                                    });
                                if (it == argObj.m_choicesInt.end())
                                {
                                    return InvalidateArgsOutOfChoice(argObj, token);
                                }
                            }
                        }
                        catch (...)
                        {
                            return InvalidateArgsCannotParse(argObj, token);
                        }
                    }
                    else
                    {
                        return InvalidateArgsCannotParse(argObj, token);
                    }
                }
                else
                {
                    return InvalidateArgsTooMany(argObj);
                }
            }
            else if (argument->second.m_type == ArgTypeCast::e_longlong)
            {
                if (argObj.m_nargs != 0)
                {
                    if (isNumber(token))
                    {
                        try
                        {
                            argument->second.m_longLong.push_back(std::stoll(token));

                            long long value = argument->second.m_longLong.back();

                            if (argObj.m_choicesLongLong.size())
                            {
                                auto it = std::find_if(argObj.m_choicesLongLong.begin(), argObj.m_choicesLongLong.end(),
                                    [value](const long long& longLongVal) -> bool
                                    {
                                        return value == longLongVal;
                                    });
                                if (it == argObj.m_choicesLongLong.end())
                                {
                                    return InvalidateArgsOutOfChoice(argObj, token);
                                }
                            }
                        }
                        catch (...)
                        {
                            return InvalidateArgsCannotParse(argObj, token);
                        }
                    }
                    else
                    {
                        return InvalidateArgsCannotParse(argObj, token);
                    }
                }
                else
                {
                    return InvalidateArgsTooMany(argObj);
                }
            }
            else if (argument->second.m_type == ArgTypeCast::e_double)
            {
                if (argObj.m_nargs != 0)
                {
                    if (isNumber(token))
                    {
                        try
                        {
                            argument->second.m_double.push_back(std::stod(token));

                            double value = argument->second.m_double.back();

                            if (argObj.m_choicesDouble.size())
                            {
                                auto it = std::find_if(argObj.m_choicesDouble.begin(), argObj.m_choicesDouble.end(),
                                    [value](const double& doubleVal) -> bool
                                    {
                                        return std::numeric_limits<double>::epsilon() >= abs(doubleVal - value);
                                    });
                                if (it == argObj.m_choicesDouble.end())
                                {
                                    return InvalidateArgsOutOfChoice(argObj, token);
                                }
                            }
                        }
                        catch (...)
                        {
                            return InvalidateArgsCannotParse(argObj, token);
                        }
                    }
                    else
                    {
                        return InvalidateArgsCannotParse(argObj, token);
                    }
                }
                else
                {
                    return InvalidateArgsTooMany(argObj);
                }
            }

            argument->second.m_count += 1;
            return true;
        }

        /// @brief Helper function that puts error about argument is out of choices list
        /// @param argObj Argument for which parsing error is generated
        /// @param token token which is not in the list
        /// @return false
        bool InvalidateArgsOutOfChoice(const Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("Value '" + token + "' is out of choices for \"" + name + "\"");

            return false;
        }

        /// @brief Helper function that puts error about argument has too many inputs
        /// @param argObj Argument for which parsing error is generated
        /// @return false
        bool InvalidateArgsTooMany(const Argument& argObj)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("too many arguments for \"" + name + "\"");

            return false;
        }

        /// @brief Helper function that puts error about argument has too many inputs
        /// @param argObj Argument for which parsing error is generated
        /// @param token token which cannot be parsed by argument rules
        /// @return false
        bool InvalidateArgsCannotParse(const Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("cannot parse [\"" + token + "\"] for  argument \"" + name + "\"");

            return false;
        }

        /// @brief Helper that reports a value rejected by a SetValidator predicate
        /// @param argObj Argument whose validator rejected the value
        /// @param token the rejected value
        /// @return false
        bool InvalidateArgsValidator(const Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString(argObj.ValidatorMessage().empty()
                ? ("Invalid value \"" + token + "\" for argument \"" + name + "\"")
                : argObj.ValidatorMessage());

            return false;
        }

        bool m_isValid = false;
        std::string m_error;
        std::map<const size_t, ArgumentParsed>  m_parsed;
        std::map<const std::string, size_t>     m_names;

        friend ArgumentParser;
    };


    /// @brief Aggregate description of a parser, for keyword-style construction
    /// -- the parser-level counterpart of NamedArgSpec / PositionalArgSpec:
    /// @code
    ///   auto parser = argparse::ArgumentParser({
    ///       .name        = "cptool",
    ///       .description = "Copy files",
    ///       .allowAbbrev = false});
    /// @endcode
    /// Also works with ordinary aggregate init in C++11/14/17. Field order
    /// follows the declaration below.
    struct ParserSpec
    {
        std::string name = "";
        std::string description = "";
        std::string epilogue = "";
        std::string usage = "";
        char        prefixChars = '-';
        bool        addHelp = true;
        bool        allowAbbrev = true;
        bool        ignoreUnknownArgs = false;
    };

    /// @brief Main class of argument parser
    /// hold all user arguments from code and orchestrate other classes
    /// in order to parse command line input
    class ArgumentParser
    {
    public:
        /// @brief Constructor for ArgumentParser
        /// @param name Program name which will appear in auto-generated help
        ArgumentParser(const std::string& name) noexcept
            : m_name(name)
        {}

        /// @brief Keyword-style constructor. See ParserSpec.
        /// @param spec aggregate of the parser's properties
        ArgumentParser(const ParserSpec& spec) noexcept
            : m_allowAbbrev(spec.allowAbbrev)
            , m_addHelp(spec.addHelp)
            , m_ignoreUnknownArgs(spec.ignoreUnknownArgs)
            , m_prefix(spec.prefixChars)
            , m_name(spec.name)
            , m_description(spec.description)
            , m_epilogue(spec.epilogue)
            , m_usage(spec.usage)
        {}

        /// @brief Overload default description for auto-generated command line
        /// @param description Text to display before the argument help ("" by default)
        /// @return reference to current parser
        ArgumentParser& SetDescription(const std::string& description) noexcept
        {
            m_description = description;
            return *this;
        }

        /// @brief Allows long options to be abbreviated if the abbreviation is unambiguous.
        /// This has two effects when enabled:
        ///  * during parsing, an unambiguous prefix of a long option is accepted
        ///    on the command line (e.g. "--verb" for "--verbose");
        ///  * for every named argument that has a long name but no explicit short
        ///    name, a single-character short name is auto-generated (when a free
        ///    letter is available) and shown in the generated help.
        /// @param allowAbbrev bool value true for allow (true by default)
        /// @return reference to current parser
        ArgumentParser& SetAllowAbbrev(bool allowAbbrev) noexcept
        {
            m_allowAbbrev = allowAbbrev;
            return *this;
        }

        /// @brief Setter to ignore unknown argument while parsing.
        /// If false parsing will be failed if parser detected unknown argument.
        /// @param ignoreUnknownArgs bool value for ignore or not (false by default)
        /// @return reference to current parser
        ArgumentParser& SetIgnoreUnknownArgs(bool ignoreUnknownArgs) noexcept
        {
            m_ignoreUnknownArgs = ignoreUnknownArgs;
            return *this;
        }

        /// @brief Add a - h / --help option to the parser
        /// @param addHelp Flag to add or not (true by default)
        /// @return reference to current parser
        ArgumentParser& SetAddHelp(bool addHelp) noexcept
        {
            m_addHelp = addHelp;
            return *this;
        }



        /// @brief This function allows to set final message after program description and before
        /// argument list description
        /// @param epilogue string of epilogue. Automatically adjusting to screen size. (empty by default)
        /// @return reference to current parser
        ArgumentParser& SetEpilogue(const std::string& epilogue) noexcept
        {
            m_epilogue = epilogue;
            return *this;
        }

        /// @brief Program usage examples
        /// @param usage the string describing the program usage. (default: generated from arguments added to parser)
        /// @return reference to current parser
        ArgumentParser& SetUsage(const std::string& usage) noexcept
        {
            m_usage = usage;
            return *this;
        }

        /// @brief Function to override default prefix. Be careful when you choosing prefix.
        /// Single prefix will be used for short names of named argument. Double prefix - for long names.
        /// Positional arguments have no any prefix
        /// @param charSym character which will be used for prefix. ('-' by default)
        /// @return 
        ArgumentParser& SetPrefixChars(const char charSym) noexcept
        {
            m_prefix = charSym;
            return *this;
        }

        /// @brief Add a named argument straight from its spec, without going
        /// through CreateNamedArgument, e.g.
        /// AddArgument({.shortName='f', .longName="file", .required=true});
        /// @param spec aggregate of the argument's properties
        void AddArgument(const NamedArgSpec& spec)
        {
            AddArgument(CreateNamedArgument(spec));
        }

        /// @brief Add a positional argument straight from its spec. See the
        /// NamedArgSpec overload.
        /// @param spec aggregate of the argument's properties
        void AddArgument(const PositionalArgSpec& spec)
        {
            AddArgument(CreatePositionalArgument(spec));
        }

        /// @brief Function to add arguments specification to command line parser
        /// @param arg Argument instance
        void AddArgument(const Argument& arg)
        {
            if (arg.m_longName.empty() && arg.m_shortName.empty() && arg.m_positionalName.empty())
            {
                throw std::runtime_error("Short,long or positional names of argument are empty.\n"
                    "At least one name should been specified.");
            }
            else if (!(arg.m_longName.empty() || arg.m_shortName.empty()) && !arg.m_positionalName.empty())
            {
                throw std::runtime_error("Positional argument " + arg.m_positionalName + " declared aside with short/long name\n"
                    "Positional argument shouldn't have any short/long name.");
            }
            else if (arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
            {
                if (arg.m_type == ArgTypeCast::e_bool)
                {
                    throw std::runtime_error("No need to declare choice for bool type");
                }
                else if (arg.m_type == ArgTypeCast::e_int && arg.m_choicesDouble.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only int choices should been declared");
                }
                else if (arg.m_type == ArgTypeCast::e_longlong && arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only long long choices should been declared");
                }
                else if (arg.m_type == ArgTypeCast::e_double && arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only double choices should been declared");
                }
                else if (arg.m_type == ArgTypeCast::e_String && arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesDouble.size())
                {
                    throw std::runtime_error("Only string choices should been declared");
                }
            }
            _addArg(arg);
        }

        /// @brief Main function of parsing argument
        /// @param args vector of input tokens
        /// @return ArgumentsObject, which contains valid ArgumentParsed if parsing successful or
        /// information about errors if not
        ArgumentsObject ParseArgs(const std::vector<std::string>& args)
        {
            std::string _pref{ m_prefix };
            std::string _doublePref{ m_prefix, m_prefix };

            // Fill in auto short names before building the internal lookup map so
            // the generated names participate in parsing.
            GenerateAbbreviations();

            for (auto& el : m_knownArgumentNames)
            {
                if (el.second.argNameType == KnownNameType::e_Short)
                {
                    m_knownArgumentNamesInternal[_pref + el.first] = el.second;
                }
                else
                {
                    m_knownArgumentNamesInternal[_doublePref + el.first] = el.second;
                }
            }

            if (m_addHelp)
            {
                bool shortHelpAlreadyExists = false, longHelpAlreadyExists = false;
                auto foundArgObject = m_knownArgumentNamesInternal.find(_pref + "h");
                shortHelpAlreadyExists = foundArgObject != m_knownArgumentNamesInternal.end();
                foundArgObject = m_knownArgumentNamesInternal.find(_doublePref + "help");
                longHelpAlreadyExists = foundArgObject != m_knownArgumentNamesInternal.end();
                if (!shortHelpAlreadyExists || !longHelpAlreadyExists)
                {
                    Argument arg = Argument::CreateNamedArgument(shortHelpAlreadyExists ? "" : "h", longHelpAlreadyExists ? "" : "help", 0);
                    arg.SetHelp("Show help!");
                    arg.SetRequired(false);
                    _addArg(arg);
                    if (!shortHelpAlreadyExists)
                    {
                        m_knownArgumentNamesInternal[_pref + arg.m_shortName] = { m_arguments.size()-1, KnownNameType::e_Short };
                    }
                    if (!longHelpAlreadyExists)
                    {
                        m_knownArgumentNamesInternal[_doublePref + arg.m_longName] = { m_arguments.size()-1, KnownNameType::e_Long };
                    }
                }
            }

            bool positionalArgsEndFlag = false;
            size_t currentArgumentObjectIndex = kSizeTypeEnd;
            // Value tokens consumed by the active option; caps fixed/'?' nargs.
            size_t currentArgConsumed = 0;
            // After a satisfied fixed option, extra bare tokens are positionals
            // -- unlike tokens after an ignored unknown option, which are dropped.
            bool spillToPositional = false;
            std::vector<std::string> positionalArgs;
            ArgumentsObject argObj;
            for (size_t i =0; i < args.size(); ++i)
            {
                const std::string& el = args[i];
                auto foundArgObject = m_knownArgumentNamesInternal.find(el);
                if (foundArgObject == m_knownArgumentNamesInternal.end() && m_allowAbbrev)
                {
                    // SetAllowAbbrev: accept an unambiguous prefix of a long option,
                    // e.g. "--verb" for "--verbose".
                    bool ambiguous = false;
                    std::string abbrev = ResolveAbbreviation(el, _doublePref, ambiguous);
                    if (ambiguous)
                    {
                        argObj.SetErrorString("Ambiguous option \"" + el
                            + "\" matches more than one argument");
                        return argObj;
                    }
                    if (!abbrev.empty())
                    {
                        foundArgObject = m_knownArgumentNamesInternal.find(abbrev);
                    }
                }
                if (foundArgObject != m_knownArgumentNamesInternal.end())
                {
                    currentArgumentObjectIndex = foundArgObject->second.position;
                    currentArgConsumed = 0;
                    spillToPositional = false;

                    Argument& argument = m_arguments[currentArgumentObjectIndex];
                    if (argument.m_nargs == 0)
                    {
                        if (!argObj.Parse(argument, currentArgumentObjectIndex, el))
                        {
                            return argObj;
                        }
                    }
                    else
                    {
                        argObj.CreateParsingStub(argument, currentArgumentObjectIndex);
                    }

                    positionalArgsEndFlag = true;
                    continue;
                }
                else if (!positionalArgsEndFlag)
                {
                    if (isNumber(el))
                    {
                        positionalArgs.push_back(el);
                    }
                    else if (el.find(_pref) == 0 || el.find(_doublePref) == 0)
                    {
                        if (!_unknownArgumentHit(argObj, i+1, currentArgumentObjectIndex, positionalArgsEndFlag, el))
                        {
                            return argObj;
                        }
                    }
                    else
                    {
                        positionalArgs.push_back(el);
                    }
                    continue;
                }
                // A negative number (e.g. "-3") is a value, not an option, even
                // though it starts with the prefix.
                else if ((el.find(_pref) == 0 || el.find(_doublePref) == 0) && !isNumber(el))
                {
                    if (!_unknownArgumentHit(argObj, i+1, currentArgumentObjectIndex, positionalArgsEndFlag, el))
                    {
                        return argObj;
                    }
                    // Tokens after an ignored unknown option are not positionals.
                    spillToPositional = false;
                    continue;
                }

                if (currentArgumentObjectIndex != kSizeTypeEnd)
                {
                    Argument& argument = m_arguments[currentArgumentObjectIndex];

                    // Bounded options ('?' -> 1, fixed -> nargs) stop once full;
                    // the rest spill to positionals. '*' / '+' stay greedy.
                    const bool bounded = argument.m_nargs >= 0
                        || argument.m_nargs == kZeroOrOneArgCount;
                    const size_t boundMax = argument.m_nargs >= 0
                        ? static_cast<size_t>(argument.m_nargs) : 1u;
                    if (bounded && currentArgConsumed >= boundMax)
                    {
                        currentArgumentObjectIndex = kSizeTypeEnd;
                        spillToPositional = true;
                        positionalArgs.push_back(el);
                    }
                    else if (!argObj.Parse(argument, currentArgumentObjectIndex, el))
                    {
                        return argObj;
                    }
                    else
                    {
                        ++currentArgConsumed;
                    }
                }
                else if (spillToPositional)
                {
                    positionalArgs.push_back(el);   // overflow after a satisfied option
                }
            }

            if (!positionalArgs.empty())
            {
                if (m_positionalArgumentNames.empty())
                {
                    argObj.SetErrorString("Unknown positional argument:" + positionalArgs.front());
                    return argObj;
                }
                // Distribute tokens across positionals argparse-style: each takes
                // between its min and max, and a variable ('*'/'+') one greedily
                // absorbs the slack while reserving the minimums that follow it.
                const size_t positionalDefsCount = m_positionalArgumentNames.size();
                const size_t totalTokens = positionalArgs.size();

                std::vector<size_t> minTokens(positionalDefsCount, 0);
                std::vector<bool>   isVariable(positionalDefsCount, false);
                size_t sumMin = 0;
                size_t sumMaxFixed = 0;
                bool   anyVariable = false;

                for (size_t k = 0; k < positionalDefsCount; ++k)
                {
                    const Argument& a = m_arguments[m_positionalArgumentNames[k].positionInArguments];
                    if (a.m_nargs == kAnyArgCount || a.m_nargs == kFromOneToInfiniteArgCount)
                    {
                        isVariable[k] = true;
                        anyVariable = true;
                        minTokens[k] = (a.m_nargs == kFromOneToInfiniteArgCount && a.m_required) ? 1 : 0;
                    }
                    else if (a.m_nargs == kZeroOrOneArgCount)
                    {
                        // '?' : zero or one
                        minTokens[k] = 0;
                        sumMaxFixed += 1;
                    }
                    else
                    {
                        const size_t n = static_cast<size_t>(a.m_nargs);
                        minTokens[k] = a.m_required ? n : 0;
                        sumMaxFixed += n;
                    }
                    sumMin += minTokens[k];
                }

                if (totalTokens < sumMin)
                {
                    argObj.SetErrorString("Too few positional arguments: required "
                        + std::to_string(sumMin) + " got " + std::to_string(totalTokens));
                    return argObj;
                }
                if (!anyVariable && totalTokens > sumMaxFixed)
                {
                    argObj.SetErrorString("Too many positional arguments!");
                    return argObj;
                }

                size_t currentTokenPosition = 0;
                for (size_t k = 0; k < positionalDefsCount; ++k)
                {
                    const auto& def = m_positionalArgumentNames[k];
                    Argument& argument = m_arguments[def.positionInArguments];

                    size_t reserveAfter = 0;
                    for (size_t j = k + 1; j < positionalDefsCount; ++j)
                    {
                        reserveAfter += minTokens[j];
                    }
                    const size_t remaining = totalTokens - currentTokenPosition;
                    const size_t avail = remaining > reserveAfter ? remaining - reserveAfter : 0;

                    size_t take;
                    if (isVariable[k])
                    {
                        take = avail;                       // greedy: grab the slack
                    }
                    else if (argument.m_nargs == kZeroOrOneArgCount)
                    {
                        take = (avail >= 1) ? 1 : 0;        // '?' : zero or one
                    }
                    else
                    {
                        // Fixed: all-or-nothing (optional takes N only if available).
                        const size_t n = static_cast<size_t>(argument.m_nargs);
                        take = (avail >= n) ? n : (argument.m_required ? n : 0);
                    }
                    if (take > remaining)   // defensive: never index past the tokens
                    {
                        take = remaining;
                    }
                    if (take == 0)
                    {
                        continue;           // absent optional positional
                    }

                    argObj.CreateParsingStub(argument, def.positionInArguments);
                    for (size_t t = 0; t < take; ++t)
                    {
                        if (!argObj.Parse(argument, def.positionInArguments, positionalArgs[currentTokenPosition]))
                        {
                            return argObj;
                        }
                        ++currentTokenPosition;
                    }
                }

                if (currentTokenPosition < totalTokens)
                {
                    argObj.SetErrorString("Too many positional arguments!");
                    return argObj;
                }
            }

            for (size_t i = 0; i < m_arguments.size(); ++i)
            {
                auto el = m_arguments[i];
                const std::string& name = el.m_longName.empty() ? (el.m_shortName.empty() ? el.m_positionalName : el.m_shortName) : el.m_longName;

                ArgumentParsed parsedArg = argObj.GetArg(name);

                if (parsedArg.GetArgumentExists())
                {
                    if (static_cast<int>(parsedArg.GetArgumentCount()) == el.m_nargs
                        || el.m_nargs == kAnyArgCount
                        || (el.m_nargs == kFromOneToInfiniteArgCount && parsedArg.GetArgumentCount() >= 1)
                        || (el.m_nargs == kZeroOrOneArgCount && parsedArg.GetArgumentCount() <= 1))
                    {
                        continue;
                    }
                    else
                    {
                        argObj.SetErrorString("Wrong arguments count for argument with name \"" + name + "\" got = " + std::to_string(parsedArg.GetArgumentCount()));
                        return argObj;
                    }
                }
                else if (el.HasDefault())
                {
                    argObj.ParseDefault(el, i);
                }
                // '*' and '?' are satisfied by zero values even if required.
                else if (el.m_required && el.m_nargs != kAnyArgCount && el.m_nargs != kZeroOrOneArgCount)
                {
                    argObj.SetErrorString("Required argument with name \"" + name + "\" does not exist");
                    return argObj;
                }
            }

            // Parsing succeeded: push values into any BindTo(...) variables.
            // Absent optionals aren't present here, so their variables stay put.
            for (size_t i = 0; i < m_arguments.size(); ++i)
            {
                const Argument& el = m_arguments[i];
                if (!el.HasBinding())
                {
                    continue;
                }
                const std::string& name = el.m_longName.empty() ? (el.m_shortName.empty() ? el.m_positionalName : el.m_shortName) : el.m_longName;
                ArgumentParsed parsedArg = argObj.GetArg(name);
                if (parsedArg.GetArgumentExists() && parsedArg.GetArgumentCount() >= 1)
                {
                    el.ApplyBinding(parsedArg);
                }
            }

            argObj.SetValid();
            return argObj;
        }

        /// @brief function which resolves unknown arguments presence
        /// @param argObj
        /// @param positionInInput
        /// @param currentArgumentObjectIndex 
        /// @param positionalArgsEndFlag 
        /// @param el 
        /// @return bool - true for ignoring, false for stopping parse 
        bool _unknownArgumentHit(ArgumentsObject& argObj, const size_t positionInInput, size_t& currentArgumentObjectIndex, bool& positionalArgsEndFlag, const std::string& el)
        {
            if (m_ignoreUnknownArgs)
            {
                currentArgumentObjectIndex = kSizeTypeEnd;
                positionalArgsEndFlag = true;
                return true;
            }
            std::stringstream ss;
            ss << "Unknown input argument: \"" << el << "\" at position " << positionInInput;
            argObj.SetErrorString(ss.str());
            return false;
        }

        /// @brief This function just converts argc and argv to vector of token
        /// @param argc count of arguments
        /// @param argv pointer to array of char*
        /// @return ArgumentsObject, which contains valid ArgumentParsed if parsing successful or
        /// information about errors if not
        ArgumentsObject ParseArgs(const int argc, char** argv)
        {

            std::vector<std::string> args;
            for (int i = 1; i < argc; ++i)
            {
                args.emplace_back(argv[i]);
            }

            return ParseArgs(args);
        }

        /// @brief This function just converts argc and argv to vector of token
        /// @param argc count of arguments
        /// @param argv pointer to array of const char*
        /// @return ArgumentsObject, which contains valid ArgumentParsed if parsing successful or
        /// information about errors if not
        ArgumentsObject ParseArgs(const int argc, const char** argv)
        {

            std::vector<std::string> args;
            for (int i = 1; i < argc; ++i)
            {
                args.emplace_back(argv[i]);
            }

            return ParseArgs(args);
        }

        /// @brief Function to get help string
        /// @param width current terminal width (80 by default)
        /// @param nameWidthPercent percentage of current width, for naming parameters (30 by default)
        /// @return help string with proper new lines
        std::string GetHelp(size_t width = kHelpWidth, size_t nameWidthPercent = kHelpNameWidthPercent)
        {
            // Ensure auto-generated short names appear in the help even when
            // GetHelp is called before ParseArgs.
            GenerateAbbreviations();

            width = width < kHelpWidth ? kHelpWidth : width;
            size_t nameWidthInHelp = nameWidthPercent * width / 100;
            width -= nameWidthInHelp;


            // NOTE: do not initialize the stream with "usage: " -- the first
            // insertion below would overwrite it (the put pointer starts at 0).
            std::stringstream usage;
            if (!m_usage.empty())
            {
                // Caller-provided usage line (SetUsage) overrides the auto-generated one.
                usage << m_usage;
            }
            else
            {
                usage << m_name << " ";
                if (m_positionalArgumentNames.size())
                {
                    for (auto& el : m_positionalArgumentNames)
                    {
                        Argument& arg = m_arguments[el.positionInArguments];
                        MakeUsageForName(arg, usage);
                    }
                }
                for (auto& el : m_arguments)
                {
                    if (el.m_positionalName.empty())
                    {
                        MakeUsageForName(el, usage);
                    }
                }
            }

            if (!m_description.empty())
            {
                AddAdditionalDescription(usage, m_description, width+nameWidthInHelp);
            }

            if (m_positionalArgumentNames.size())
            {
                usage << "\n\n" << "positional arguments:\n\n";
                for (auto& el : m_positionalArgumentNames)
                {
                    Argument& arg = m_arguments[el.positionInArguments];
                    MakeDescriptionForArg(arg, usage, nameWidthInHelp, width);
                }
            }

            if (m_arguments.size() > m_positionalArgumentNames.size())
            {
                usage << "\n\n" << "named arguments:\n\n";
                for (auto& el : m_arguments)
                {
                    if (el.m_positionalName.empty())
                    {
                        MakeDescriptionForArg(el, usage, nameWidthInHelp, width);
                    }
                }
            }

            if (!m_epilogue.empty())
            {
                AddAdditionalDescription(usage, m_epilogue, width+nameWidthInHelp);
            }

            return TrimTrailingSpacesPerLine(usage.str());
        }

        /// @brief Removes trailing spaces/tabs from every line of the help text.
        /// The column-based layout leaves padding at the end of many lines; this
        /// keeps the rendered help clean without touching the wrapping logic.
        /// @param text help text possibly containing trailing whitespace
        /// @return text with per-line trailing whitespace removed
        static std::string TrimTrailingSpacesPerLine(const std::string& text)
        {
            std::string result;
            result.reserve(text.size());
            size_t lineStart = 0;
            while (lineStart <= text.size())
            {
                size_t nl = text.find('\n', lineStart);
                size_t lineEnd = (nl == std::string::npos) ? text.size() : nl;
                size_t last = lineEnd;
                while (last > lineStart && (text[last - 1] == ' ' || text[last - 1] == '\t'))
                {
                    --last;
                }
                result.append(text, lineStart, last - lineStart);
                if (nl == std::string::npos)
                {
                    break;
                }
                result.push_back('\n');
                lineStart = nl + 1;
            }
            return result;
        }

    private:


        /// @brief Resolves an unambiguous long-option abbreviation (SetAllowAbbrev).
        /// Given a token like "--verb", finds the single long option whose
        /// prefixed name ("--verbose") begins with it.
        /// @param token the (double-prefixed) token typed on the command line
        /// @param doublePref the double-prefix string (e.g. "--")
        /// @param ambiguous set to true if the token is a prefix of more than one
        /// long option; in that case an empty string is returned.
        /// @return the full internal key of the unique match, or "" if none/ambiguous.
        std::string ResolveAbbreviation(
            const std::string& token, const std::string& doublePref, bool& ambiguous)
        {
            ambiguous = false;
            // Only long options (prefixed with the double prefix) can be abbreviated,
            // and the token must be a strict, non-empty prefix.
            if (token.size() <= doublePref.size()
                || token.compare(0, doublePref.size(), doublePref) != 0)
            {
                return std::string();
            }

            std::string match;
            for (auto it = m_knownArgumentNamesInternal.begin();
                it != m_knownArgumentNamesInternal.end(); ++it)
            {
                if (it->second.argNameType == KnownNameType::e_Long
                    && it->first.size() > token.size()
                    && it->first.compare(0, token.size(), token) == 0)
                {
                    if (!match.empty())
                    {
                        ambiguous = true;
                        return std::string();
                    }
                    match = it->first;
                }
            }
            return match;
        }

        /// @brief Auto-generates single-character short names for named arguments
        /// that have a long name but no explicit short name, when SetAllowAbbrev
        /// is enabled. For each such argument the first free alphanumeric letter of
        /// its long name is chosen; if every letter is already taken the argument
        /// simply keeps no short name. The generated names are registered so they
        /// work both for parsing and for the auto-generated help output.
        /// Runs at most once (guarded by m_abbrevGenerated).
        void GenerateAbbreviations()
        {
            if (!m_allowAbbrev || m_abbrevGenerated)
            {
                return;
            }
            m_abbrevGenerated = true;

            for (size_t i = 0; i < m_arguments.size(); ++i)
            {
                Argument& arg = m_arguments[i];
                // Only named arguments with a long name and no short name qualify.
                if (arg.m_longName.empty() || !arg.m_shortName.empty()
                    || !arg.m_positionalName.empty())
                {
                    continue;
                }

                for (size_t c = 0; c < arg.m_longName.size(); ++c)
                {
                    const char ch = arg.m_longName[c];
                    const bool isAlnum = (ch >= 'a' && ch <= 'z')
                        || (ch >= 'A' && ch <= 'Z')
                        || (ch >= '0' && ch <= '9');
                    if (!isAlnum)
                    {
                        continue;
                    }

                    const std::string candidate(1, ch);
                    // "h" is reserved for the auto-added -h/--help option.
                    if (m_addHelp && candidate == "h")
                    {
                        continue;
                    }
                    if (m_knownArgumentNames.find(candidate) != m_knownArgumentNames.end())
                    {
                        continue;
                    }

                    arg.m_shortName = candidate;
                    m_knownArgumentNames[candidate] = { i, KnownNameType::e_Short };
                    break;
                }
            }
        }

        /// @brief Private function which is called when AddArgument function
        /// succeed without errors
        /// @param arg Argument object after all validations check
        void _addArg(const Argument& arg)
        {
            size_t currentSize = m_arguments.size();
            if (!arg.m_shortName.empty())
            {
                if (m_knownArgumentNames.find(arg.m_shortName) == m_knownArgumentNames.end())
                {
                    m_knownArgumentNames[arg.m_shortName] = { currentSize, KnownNameType::e_Short };
                }
                else
                {
                    throw std::runtime_error("Short name \"" + arg.m_shortName + "\" already exists");
                }
            }
            if (!arg.m_longName.empty())
            {
                if (m_knownArgumentNames.find(arg.m_longName) == m_knownArgumentNames.end())
                {
                    m_knownArgumentNames[arg.m_longName] = { currentSize, KnownNameType::e_Long };
                }
                else
                {
                    throw std::runtime_error("Long name \"" + arg.m_longName + "\" already exists");
                }
            }
            if (!arg.m_positionalName.empty())
            {
                auto it = std::find_if(m_positionalArgumentNames.begin(), m_positionalArgumentNames.end(), [&arg](PositionalNamesStruct& posarg)->bool {return posarg.argName == arg.m_positionalName; });
                if (it == m_positionalArgumentNames.end())
                {
                    m_positionalArgumentNames.emplace_back(m_arguments.size(), arg.m_positionalName);
                }
                else if (arg.m_required && (arg.m_nargs == 0 || arg.m_nargs == kAnyArgCount))
                {
                    throw std::runtime_error("Required positional argument with name \"" + arg.m_positionalName + "\" cannot be with zero count");
                }
                else if (!arg.m_required && arg.m_nargs != 1)
                {
                    throw std::runtime_error("Non required positional argument with name \"" + arg.m_positionalName + "\" should be with count 1");
                }
                else
                {
                    throw std::runtime_error("Positional name \"" + arg.m_positionalName + "\" already exists");
                }
            }

            m_arguments.emplace_back(arg);
        }

        /// @brief Private function which generates usage according to all argument of program
        /// @param arg Argument instance
        /// @param usage out parameter, which returns usage.
        void MakeUsageForName(Argument& arg, std::stringstream& usage)
        {
            if (!arg.m_required)
            {
                usage << "[";
            }
            std::stringstream showName;
            if (arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
            {
                showName << "{";
                if (arg.m_type == ArgTypeCast::e_String)
                {
                    MakeChoicesToString<std::string>(showName, arg.m_choicesString);
                }
                else if (arg.m_type == ArgTypeCast::e_int)
                {
                    MakeChoicesToString<int>(showName, arg.m_choicesInt);
                }
                else if (arg.m_type == ArgTypeCast::e_double)
                {
                    MakeChoicesToString<double>(showName, arg.m_choicesDouble);
                }
                else if (arg.m_type == ArgTypeCast::e_longlong)
                {
                    MakeChoicesToString<long long>(showName, arg.m_choicesLongLong);
                }
                showName << "}";
            }
            else
            {
                if (!arg.m_positionalName.empty())
                {
                    showName << arg.m_positionalName;
                }
                else if (!arg.m_shortName.empty())
                {
                    showName << arg.m_shortName;
                }
                else
                {
                    showName << arg.m_longName;
                }
            }
            if (!arg.m_positionalName.empty())
            {
                usage << showName.str();
            }
            if (!arg.m_shortName.empty())
            {
                usage << m_prefix << arg.m_shortName;
            }

            if (!arg.m_longName.empty())
            {
                if (!arg.m_shortName.empty())
                {
                    usage << ",";
                }
                usage << m_prefix << m_prefix << arg.m_longName;
            }

            if (arg.m_nargs == kAnyArgCount)
            {
                usage << " [" << showName.str() << "[" << showName.str() << " ...]]";
            }
            else if (arg.m_nargs == kFromOneToInfiniteArgCount)
            {
                usage << " [" << showName.str() << " ...]";
            }
            else if (arg.m_nargs != 0)
            {
                usage << " [";
                usage << showName.str();
                for (size_t i = 1; i < static_cast<size_t>(arg.m_nargs); ++i)
                {
                    usage << " " << showName.str();
                }
                usage << "]";
            }
            if (!arg.m_required)
            {
                usage << "]";
            }
            // Always separate tokens with exactly one trailing space. This also
            // keeps flags (nargs == 0) from gluing onto the next token.
            usage << " ";
        }

        /// @brief Private function which generates description for every Argument instance which added to parser
        /// This function called from GetHelp functions chain in case if m_addHelp is true.
        /// @param arg Argument instance
        /// @param description string stream to which add description
        /// @param nameLen actual length of names column in usage. If names are longer then nameLen new line will be added before first line of description/
        /// nameLen is calculated according to terminal width and name nameWidthPercent from GetHelp input parameters
        /// @param descLen length of description column of usage.
        void MakeDescriptionForArg(Argument& arg, std::stringstream& description, size_t nameLen, size_t descLen)
        {
            std::stringstream showName;
            std::stringstream showDesc;
            if (!arg.m_positionalName.empty())
            {
                if (arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
                {
                    showName << "{";
                    if (arg.m_type == ArgTypeCast::e_String)
                    {
                        MakeChoicesToString<std::string>(showName, arg.m_choicesString);
                    }
                    else if (arg.m_type == ArgTypeCast::e_int)
                    {
                        MakeChoicesToString<int>(showName, arg.m_choicesInt);
                    }
                    else if (arg.m_type == ArgTypeCast::e_double)
                    {
                        MakeChoicesToString<double>(showName, arg.m_choicesDouble);
                    }
                    else if (arg.m_type == ArgTypeCast::e_longlong)
                    {
                        MakeChoicesToString<long long>(showName, arg.m_choicesLongLong);
                    }
                    showName << "}";
                }
                else
                {
                    showName << arg.m_positionalName;
                }
            }
            else if (!arg.m_shortName.empty())
            {
                showName << m_prefix << arg.m_shortName;
                if (!arg.m_longName.empty())
                {
                    showName << "," << m_prefix << m_prefix << arg.m_longName;
                }
            }
            else
            {
                // Long name only: no short name, so no leading comma.
                showName << m_prefix << m_prefix << arg.m_longName;
            }

            showDesc << arg.m_help;
            if (arg.m_nargs)
            {
                showDesc << (arg.m_help.empty() ? "" : " ") << "Type: " << m_enumToString[arg.m_type] << ". ";
            }

            if ((!arg.m_shortName.empty() || !arg.m_longName.empty())
                && (arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size()))
            {
                showDesc << " Choices:";
                if (arg.m_type == ArgTypeCast::e_String)
                {
                    MakeChoicesToString<std::string>(showDesc, arg.m_choicesString);
                }
                else if (arg.m_type == ArgTypeCast::e_int)
                {
                    MakeChoicesToString<int>(showDesc, arg.m_choicesInt);
                }
                else if (arg.m_type == ArgTypeCast::e_double)
                {
                    MakeChoicesToString<double>(showDesc, arg.m_choicesDouble);
                }
                else if (arg.m_type == ArgTypeCast::e_longlong)
                {
                    MakeChoicesToString<long long>(showDesc, arg.m_choicesLongLong);
                }
                showDesc << ". ";
            }

            showDesc << (arg.m_nargs ? "Args count: " : "");
            switch (arg.m_nargs)
            {
            case kAnyArgCount:
                showDesc << "any. ";
                break;
            case kFromOneToInfiniteArgCount:
                showDesc << " at least one. ";
                break;
            case 0:
                break;
            default:
                showDesc << arg.m_nargs << " ";
                break;
            }
            description << showName.str();
            size_t currentLen = getStringStreamLength(showName);

            size_t spaceFillerSize = nameLen - currentLen;
            if (currentLen >= nameLen - 1)
            {
                description << "\n";
                spaceFillerSize = nameLen;
            }

            // generate space after names printed
            std::string filler(spaceFillerSize, ' ');
            description << filler;

            // generate space for new line of description
            currentLen = 0;

            filler = std::string(nameLen, ' ');

            for (std::string s; showDesc >> s; )
            {
                if (currentLen > descLen)
                {
                    currentLen = 0;
                    description << "\n" << filler;
                }
                description << s << " ";
                currentLen += s.size() + 1;
            }

            description << "\n";
        }


        /// @brief Private function to add epilogue or program description for program help if
        /// auto-generated help is requested.
        /// @param description Current description stream
        /// @param additionalDesc Description to add
        /// @param descLen length of terminal
        void AddAdditionalDescription(std::stringstream& description, const std::string& additionalDesc, size_t descLen)
        {
            description << "\n";
            size_t currentLen = 0;
            std::stringstream buffer{ additionalDesc };
            for (std::string s; buffer >> s; )
            {
                if (currentLen > descLen)
                {
                    currentLen = 0;
                    description << "\n";
                }
                description << s << " ";
                currentLen += s.size() + 1;
            }
        }


        /// @brief Private function which generates choices for Argument instance description, if argument has
        /// any choices
        /// @tparam T type of choices vector
        /// @param outSstream output string stream
        /// @param choices vector of available choices.
        template<typename T>
        void MakeChoicesToString(std::stringstream& outSstream, std::vector<T>& choices)
        {
            outSstream << choices.front();
            for (size_t i = 1; i < choices.size(); ++i)
            {
                outSstream << ", " << choices[i];
            }
        }

    private:
        enum class KnownNameType :int
        {
            e_Short,
            e_Long
        };
        struct KnownNamesStruct
        {
            size_t position;
            KnownNameType argNameType;
        };

        struct PositionalNamesStruct
        {
            PositionalNamesStruct(const size_t position, const std::string& name)
                :positionInArguments(position)
                , argName(name)
            {}
            size_t positionInArguments;
            std::string argName;
        };

    private:
        /// @brief allow generate short names for named arguments, short name not preset
        bool        m_allowAbbrev = true;
        /// @brief guards GenerateAbbreviations so it runs at most once
        bool        m_abbrevGenerated = false;
        /// @brief generate help automatically
        bool        m_addHelp = true;
        /// @brief fail parsing if unknown argument is passed to command line
        bool        m_ignoreUnknownArgs = false;
        /// @brief prefix for short for named arguments
        char        m_prefix = '-';

        /// @brief name of program which would be occur in command line
        /// if auto generated help is required
        std::string m_name;
        /// @brief description of program which should be shown in help
        /// if auto generated help is required
        std::string m_description{ "" };
        /// @brief epilogue of program which should be placed after arguments list
        /// if auto generated help is required
        std::string m_epilogue{ "" };
        /// @brief usage examples of program
        /// if auto generated help is required
        std::string m_usage{ "" };
        /// @brief holder of defined arguments
        std::vector<Argument> m_arguments;
        /// @brief holder positional arguments with they position
        std::vector<PositionalNamesStruct>      m_positionalArgumentNames;
        /// @brief map with arguments which added explicitly
        std::map<std::string, KnownNamesStruct> m_knownArgumentNames;
        /// @brief generated map with arguments after add short names if needed and so on
        /// actual map which will be used for parsing
        std::map<std::string, KnownNamesStruct> m_knownArgumentNamesInternal;

        std::map<const ArgTypeCast, const std::string> m_enumToString
        {
            {ArgTypeCast::e_String,     "STRING" },
            {ArgTypeCast::e_int,        "INT" },
            {ArgTypeCast::e_longlong,   "LONG_LONG"},
            {ArgTypeCast::e_double,     "DOUBLE"},
            {ArgTypeCast::e_bool,       "BOOL"}
        };
    };
}