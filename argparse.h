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

#if __cplusplus > 201402L || _MSVC_LANG > 201402L
#include <any>
#endif

/// @brief namespace of argument parser constans and Classes
/// can be changed if ARGPARSE_NAMESPACE_NAME macro specified during compilation.
/// By default is namespace name is "argparse"
namespace ARGPARSE_NAMESPACE_NAME
{
    namespace /// anonymous namespace for internal uasge
    {
        const size_t kSizeTypeEnd = static_cast<size_t>(-1);
        const size_t kHelpWidth = 80;
        const size_t kHelpNameWidthPercent = 30;

        bool isNumber(const std::string& inStr)
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

        size_t getStringStreamLength(std::stringstream& showDesc)
        {
            showDesc.seekp(0, std::ios::end);
            return showDesc.tellp();
        }
    }


    /// @brief Supported types for argument
    /// If neede type is not in this list, then just use e_String
    enum class ArgTypeCast : int
    {
        e_String,
        e_int,
        e_longlong,
        e_double,
        e_bool
    };

    /// @brief constatnt to indicate arguments with vaious
    /// count from 0 to infinite
    const int kAnyArgCount = -1;
    /// @brief constatnt to indicate arguments with vaious
    /// count from 1 to infinite
    const int kFromOneToInfinteArgCount = -2;


    class ArgumentParser;
    class ArgumentsObject;

    /// @brief This class represents argument configuration
    /// which should be passed to ArgumentParser objects instance
    class Argument
    {
        /// @brief Default constructor. Private!
        /// @param positionalName represents name for positional argument.
        /// @param shortName represents short name for named argument which should be passed with one prefix.
        /// @param longName represents long name for named atgument which should be passed with double prefix.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfinteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        Argument(const std::string& positionalName = "",
            const std::string& shortName = "",
            const std::string& longName = "",
            const int argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "")
            : m_required(required)
            , m_nargs(argsCount)
            , m_type(argType)
            , m_positionalName(positionalName)
            , m_shortName(shortName)
            , m_longName(longName)
            , m_help(help)
        {}
    public:

        /// @brief Default constructor positional arguments. You can use class Setters or pass your own values to public members directly.
        /// @param shortName represents short name for named argument which should be passed with one prefix.
        /// @param longName represents long name for named atgument which should be passed with double prefix.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfinteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        static Argument CreateNamedArgument(const std::string& shortName = "",
            const std::string& longName = "",
            const int argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "")
        {
            return Argument("", shortName, longName, argsCount, argType, required, help);
        }

        /// @brief Default function for named arguments. You can use class Setters or pass your own values to public members directly.
        /// @param positionalName represents name for positional argument.
        /// @param argsCount integer number. You can use "kAnyArgCount", "kFromOneToInfinteArgCount" for non strict count or any int constant.
        /// @param argType type of argument. Defined via enum. Supported types are: int, long long, double and bool and string for all other cases.
        /// @param required Is argument required. Will fail parsing, if required argument are not present.
        /// @param help Your own custom help string start.
        static Argument CreatePositionalArgument(const std::string& positionalName = "",
            const int argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "")
        {
            return Argument(positionalName, "", "", argsCount, argType, required, help);
        }

        /// @brief required flag argument
        /// If required argument is not set in comman line
        /// then parcing will fail.
        /// All Arguments are required by default
        bool m_required = true;

        /// @brief Setter function to flag,
        /// @param required - bool value to idnicate is argument required or not
        /// @return reference to current argument
        Argument& SetRequired(bool required)
        {
            m_required = required;
            return *this;
        }

        /// @brief variable that indicates count of argument in input
        /// use "kAnyArgCount" or "kFromOneToInfinteArgCount" constants
        /// for arguments with variable count. Any other aruments count
        /// will be passed as stict arguments count.
        /// 0 is for flags (arguments that doesn't carry any data)
        /// set to 1 by default.
        int m_nargs = 1;

        /// @brief setter function for m_nargs with desired ammount
        /// @param ammount int value that indicates  ammout of argument.
        /// Coud be "kAnyArgCount" or "kFromOneToInfinteArgCount", 0 or any other positive integer.
        /// @return reference to current argument
        Argument& SetNumberOfArguments(int ammount)
        {
            m_nargs = ammount;
            return *this;
        }

        /// @brief Handy setter for argument count with self declarated name
        /// @return reference to current argument
        Argument& SetAnyNumberOfArgumentsButAtleastOne()
        {
            m_nargs = kFromOneToInfinteArgCount;
            return *this;
        }

        /// @brief Handy setter for argument count with self declarated name
        /// @return reference to current argument
        Argument& SetAnyNumberOfArguments()
        {
            m_nargs = kAnyArgCount;
            return *this;
        }

        /// @brief Handy setter for argument count with self declarated name
        /// @return reference to current argument
        Argument& SetArgumentIsFlag()
        {
            m_nargs = 0;
            return *this;
        }

        /// @brief Variable that hold type of argument.
        /// string by defalut
        ArgTypeCast m_type = ArgTypeCast::e_String;

        /// @brief Setter function for typoe of current argument
        /// @param argType setter for type of current argument data.
        /// Any non string types will be casted while parcing.
        /// @return reference to current argument
        Argument& SetType(ArgTypeCast argType)
        {
            m_type = argType;
            return *this;
        }

        /// @brief name of positional argument
        /// positional arguments name used only to acces desired argument from code
        std::string m_positionalName = "";

        /// @brief Handy setter for positional argument
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetPositionalName(const std::string& name)
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
        /// Can be autogenerated if possible when m_allowAbbrev in ArgymentParsed set to true.
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetShortName(const std::string& name)
        {
            m_shortName = name;
            return *this;
        }
        
        /// @brief arguments long name.
        /// for non positional arguments only
        /// agument long name start with double prefix
        std::string m_longName = "";

        /// @brief Handy setter for long named argument. 
        /// Should been used with double prefix in command line.
        /// Can be used for autogeneration of short name if it possible forpossible 
        /// and m_allowAbbrev in ArgymentParsed set to true.
        /// @param name name for positional argument. Empty by default
        /// @return reference to current argument
        Argument& SetLongName(const std::string& name)
        {
            m_longName = name;
            return *this;
        }

        /// @brief additional help info for argument
        /// Will be part of generated help
        std::string m_help = "";

        /// @brief Handy setter for additiona help
        /// @param help String with additional help. Empty by deafult.
        /// @return reference to current argument
        Argument& SetHelp(const std::string& help)
        {
            m_help = help;
            return *this;
        }
        
        /// @brief vector of strings to validate arguments input data.
        /// Empty by default. Will fail parsing if string not is in input list
        std::vector<std::string> m_choicesString = {};

        /// @brief Handy setter of valid choices for arguments with string type
        /// @param choices vector or initializer list of valid strings
        /// @return reference to current argument
        Argument& SetChoices(const std::vector<std::string>& choices)
        {
            if (m_type != ArgTypeCast::e_String)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesString = choices;
            return *this;
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


        /// @brief Getter to indicate does argument has any dafult value
        /// @return reference to current argument
        bool HasDefault() const
        {
            return m_hasDefault;
        }

    private:
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
    /// Short name could be autogenerated if possible when m_allowAbbrev in ArgymentParsed set to true)
    /// @param longName Full name of argument
    /// @param argsCount Count of arguments
    /// (use kFromOneToInfinteArgCount or kAnyArgCount for various arguments count)
    /// @param argType e_String, e_int, e_longlong, e_double, e_bool
    /// @param required Marker if argument should be passed or ignored if missed.
    /// @param help Initial part of help for current argument in case of autogenerated help.
    /// @return instance of Argument
    Argument CreateNamedArgument(const std::string& shortName = "",
        const std::string& longName = "",
        const int argsCount = 1,
        ArgTypeCast argType = ArgTypeCast::e_String,
        const bool required = true,
        const std::string& help = "")
    {
        return Argument::CreateNamedArgument(shortName, longName, argsCount, argType, required, help);
    }

    /// @brief Helper function to create positional argument
    /// @param positionalName Name of positional argument to access from code.
    /// @param argsCount Count of arguments
    /// (use kFromOneToInfinteArgCount or kAnyArgCount for various arguments count)
    /// @param argType e_String, e_int, e_longlong, e_double, e_bool
    /// @param required Marker if argument should be passed or ignored if missed.
    /// @param help Initial part of help for current argument in case of autogenerated help.
    /// @return instance of Argument
    Argument CreatePositionalArgument(const std::string& positionalName = "",
        const int argsCount = 1,
        ArgTypeCast argType = ArgTypeCast::e_String,
        const bool required = true,
        const std::string& help = "")
    {
        return Argument::CreatePositionalArgument(positionalName, argsCount, argType, required, help);
    }

    /// @brief Class which represent actual parsed argument in case of successfully parsing
    class ArgumentParsed
    {
    public:
        /// @brief Does argument exists. Neede to check in case
        /// when argument is not required
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
        const bool GetAsBool() const
        {
            return m_bool.front();
        }

        /// @brief Get result as single int for int type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return int value of argument
        const int GetAsInt() const
        {
            return m_int.front();
        }

        /// @brief Get result as single long long for long long type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return long long value of argument
        const long long GetAsLongLong() const
        {
            return m_longLong.front();
        }


        /// @brief Get result as single double for double type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return double value of argument
        const double GetAsDouble() const
        {
            return m_double.front();
        }


        /// @brief Get result as single string for string type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return string value of argument
        const std::string& GetAsString() const
        {
            return m_string.front();
        }

        /// @brief Get result as vector bool for bool type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector bool value of argument
        const std::vector<bool>& GetAsVecBool() const
        {
            return m_bool;
        }

        /// @brief Get result as vector int for int type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector int value of argument
        const std::vector<int>& GetAsVecInt() const
        {
            return m_int;
        }

        /// @brief Get result as vector long long for long long type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector long long value of argument
        const std::vector<long long>& GetAsVecLongLong() const
        {
            return m_longLong;
        }

        /// @brief Get result as vector double for double type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector double value of argument
        const std::vector<double>& GetAsVecDouble() const
        {
            return m_double;
        }

        /// @brief Get result as vector string for string type arguments. Added for c++11 support.
        /// Starting from c++17 you can use Get()
        /// @return vector string value of argument
        const std::vector<std::string>& GetAsVecString() const
        {
            return m_string;
        }

#if __cplusplus > 201402L || _MSVC_LANG > 201402L
        /// @brief Function to get actual argument value
        /// @return std::any: could be bool, int, long long, double, string 
        /// and vector variants of same types regarding of arguments type.
        std::any Get()
        {
            if (m_type == ArgTypeCast::e_String)
            {
                if (m_count == 1)
                {
                    return m_string.front();
                }
                else
                {
                    return m_string;
                }
            }
            else if (m_type == ArgTypeCast::e_int)
            {
                if (m_count == 1)
                {
                    return m_int.front();
                }
                else
                {
                    return m_int;
                }
            }
            else if (m_type == ArgTypeCast::e_longlong)
            {
                if (m_count == 1)
                {
                    return m_longLong.front();
                }
                else
                {
                    return m_longLong;
                }
            }
            else if (m_type == ArgTypeCast::e_double)
            {
                if (m_count == 1)
                {
                    return m_double.front();
                }
                else
                {
                    return m_double;
                }
            }
            else if (m_type == ArgTypeCast::e_bool)
            {
                if (m_count == 1)
                {
                    return m_bool.front();
                }
                else
                {
                    return m_bool;
                }
            }
        }

#endif // __cplusplus >= 


    protected:

        ArgumentParsed() {}

        bool        m_exists{ false };
        ArgTypeCast m_type{ ArgTypeCast::e_String };
        size_t      m_count{ 0 };

        std::vector<bool>        m_bool = {};
        std::vector<int>         m_int = {};
        std::vector<long long>   m_longLong = {};
        std::vector<double>      m_double = {};
        std::vector<std::string> m_string = {};

        friend ArgumentsObject;
    };

    /// @brief Cass that carries result of real parsing
    /// Indicates if parsing is sucessfull and allows to get ArgumentParsed object in that case.
    /// In case of parsing failure provides error string for first error.
    class ArgumentsObject
    {
    public:
        /// @brief Indicates if parsing was successfull
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
        const size_t ParsedArgsCount() const
        {
            return m_parsed.size();
        }

        /// @brief Getter function to get ArgumentParsed object 
        /// @param name Name of argument object, could be short name, long name or positional name
        /// @return Empty argument if argument with given name doesnt exists or real result if exists.
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

            if (argument->second.m_type == ArgTypeCast::e_String)
            {
                if (argObj.m_nargs != 0)
                {
                    if (argObj.m_choicesString.size())
                    {
                        auto it = std::find_if(argObj.m_choicesString.begin(), argObj.m_choicesString.end(),
                            [&token](const std::string& str) -> bool
                            {
                                return token == str;
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

                            long long value = argument->second.m_int.back();

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
        bool InvalidateArgsOutOfChoice(const argparse::Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("Value '" + token + "' is out of choices for" + name);

            return false;
        }

        /// @brief Helper function that puts error about argument has too many inputs
        /// @param argObj Argument for which parsing error is generated
        /// @return false
        bool InvalidateArgsTooMany(const argparse::Argument& argObj)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("too many argument for " + name);

            return false;
        }

        /// @brief Helper function that puts error about argument has too many inputs
        /// @param argObj Argument for which parsing error is generated
        /// @param token token which cannot be parsed by argument rules
        /// @return false
        bool InvalidateArgsCannotParse(const argparse::Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("cannot parse " + token + " for " + name);

            return false;
        }

        bool m_isValid = false;
        std::string m_error;
        std::map<const size_t, ArgumentParsed>  m_parsed;
        std::map<const std::string, size_t>     m_names;

        friend ArgumentParser;
    };


    /// @brief Main class of argument parser
    /// hold all user arguments from code and orchestrate other classes 
    /// in order to parse command line input
    class ArgumentParser
    {
    public:
        /// @brief Constructor for ArgumentParser
        /// @param name Programm name which will appear in autogenerated help
        /// @return 
        ArgumentParser(const std::string& name) noexcept
            : m_name(name)
        {}

        /// @brief Overload default description for autogenerated command line
        /// @param description Text to display before the argument help ("" by default)
        /// @return reference to current parser
        ArgumentParser& SetDescription(const std::string& description) noexcept
        {
            m_description = description;
            return *this;
        }

        /// @brief Allows long options to be abbreviated if the abbreviation is unambiguous.
        /// @param allowAbbrev bool value true for allow (true by default)
        /// @return reference to current parser
        ArgumentParser& SetAllowAbbrev(bool allowAbbrev) noexcept
        {
            m_allowAbbrev = allowAbbrev;
            return *this;
        }

        /// @brief Settert to ignore uknown argument while parsing.
        /// If false pasing wiil be failed if parser detected uknown argument.
        /// @param ignoreUnknownArgs bool value for ignore or not (false by default)
        /// @return reference to current parser
        ArgumentParser& SetIgnoreUknownArgs(bool ignoreUnknownArgs) noexcept
        {
            m_ignoreUknownArgs = ignoreUnknownArgs;
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
        /// @param epilog string of epilog. Automatically ajusting to screen size. (empty by default)
        /// @return reference to current parser
        ArgumentParser& SetEpilog(const std::string& epilog) noexcept
        {
            m_epilog = epilog;
            return *this;
        }

        /// @brief Programm usage examples
        /// @param usage the string describing the program usage. (default: generated from arguments added to parser)
        /// @return reference to current parser
        ArgumentParser& SetUsage(const std::string& usage) noexcept
        {
            m_usage = usage;
            return *this;
        }

        /// @brief Function to override default prefix. Be careful when you chosing prefix.
        /// Single prefix will be used for short names of named arguemtn. Double prefix - for long names.
        /// Positional arguments have no any prefix
        /// @param charSym character which will be used for prefix. ('-' by default)
        /// @return 
        ArgumentParser& SetPrefixChars(const char charSym) noexcept
        {
            m_prefix = charSym;
            return *this;
        }

        /// @brief Function to add aruments specification to command line parser
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
                throw std::runtime_error("Positional argument " + arg.m_positionalName + " decalred aside with short/long name\n"
                    "Positional argument shouldn't have any short/long name.");
            }
            else if (arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
            {
                if (arg.m_type == ArgTypeCast::e_bool)
                {
                    throw std::runtime_error("No need to declaire choice for bool type");
                }
                else if (arg.m_type == ArgTypeCast::e_int && arg.m_choicesDouble.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only int choices should been declaired");
                }
                else if (arg.m_type == ArgTypeCast::e_longlong && arg.m_choicesDouble.size() + arg.m_choicesInt.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only long long choices should been declaired");
                }
                else if (arg.m_type == ArgTypeCast::e_double && arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesString.size())
                {
                    throw std::runtime_error("Only double choices should been declaired");
                }
                else if (arg.m_type == ArgTypeCast::e_String && arg.m_choicesInt.size() + arg.m_choicesLongLong.size() + arg.m_choicesDouble.size())
                {
                    throw std::runtime_error("Only string choices should been declaired");
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
                    m_arguments.push_back(arg);
                }
            }

            if (m_allowAbbrev)
            {
                AddShortNames();
            }


            bool positionalArgsEndFlag = false;
            size_t currentArgumentObjectIndex = kSizeTypeEnd;
            std::vector<std::string> positionalArgs;
            ArgumentsObject argObj;
            for (auto& el : args)
            {

                const auto foundArgObject = m_knownArgumentNamesInternal.find(el);
                if (foundArgObject != m_knownArgumentNamesInternal.end())
                {
                    currentArgumentObjectIndex = foundArgObject->second.position;

                    Argument& argument = m_arguments[currentArgumentObjectIndex];
                    if (argument.m_nargs == 0)
                    {
                        if (!argObj.Parse(argument, currentArgumentObjectIndex, el))
                        {
                            return argObj;
                        }
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
                        _uknownArgumentHit(currentArgumentObjectIndex, positionalArgsEndFlag, el);
                    }
                    else
                    {
                        positionalArgs.push_back(el);
                    }
                    continue;
                }
                else if (el.find(_pref) == 0 || el.find(_doublePref) == 0)
                {
                    _uknownArgumentHit(currentArgumentObjectIndex, positionalArgsEndFlag, el);
                    continue;
                }

                if (currentArgumentObjectIndex != kSizeTypeEnd)
                {
                    Argument& argument = m_arguments[currentArgumentObjectIndex];

                    if (!argObj.Parse(argument, currentArgumentObjectIndex, el))
                    {
                        return argObj;
                    }
                }
            }

            if (!positionalArgs.empty())
            {
                if (m_positionalArgumentNames.empty())
                {
                    argObj.SetErrorString("Uknown positional argument:" + positionalArgs.front());
                    return argObj;
                }
                size_t minimumRequiredPositinalCount = 0;
                size_t infiniteRequiredPositionalCount = 0;
                size_t optionalPositinalCount = 0;

                for (auto& el : m_positionalArgumentNames)
                {
                    if (m_arguments[el.positionInArguments].m_required)
                    {
                        minimumRequiredPositinalCount += m_arguments[el.positionInArguments].m_nargs == kFromOneToInfinteArgCount ? 1 : m_arguments[el.positionInArguments].m_nargs;
                        infiniteRequiredPositionalCount = m_arguments[el.positionInArguments].m_nargs == kFromOneToInfinteArgCount;
                    }
                    else
                    {
                        ++optionalPositinalCount;
                    }
                }
                if (minimumRequiredPositinalCount > positionalArgs.size())
                {
                    argObj.SetErrorString("Too few positional arguments: required " + std::to_string(positionalArgs.size()) + " got " + std::to_string(minimumRequiredPositinalCount));
                    return argObj;
                }

                size_t totalTokensForRequiredNargs = 1;
                size_t additionalTokensForFirstRequiredNarg = 0;
                size_t howMuchOptionalArgsCanBeParsed = positionalArgs.size() - minimumRequiredPositinalCount;
                if (howMuchOptionalArgsCanBeParsed > optionalPositinalCount)
                {
                    if (infiniteRequiredPositionalCount == 0)
                    {
                        argObj.SetErrorString("Too many positional arguments!");
                        return argObj;
                    }
                    else
                    {
                        totalTokensForRequiredNargs = (howMuchOptionalArgsCanBeParsed - optionalPositinalCount) / infiniteRequiredPositionalCount;
                        additionalTokensForFirstRequiredNarg = (howMuchOptionalArgsCanBeParsed - optionalPositinalCount) % infiniteRequiredPositionalCount;
                    }
                    howMuchOptionalArgsCanBeParsed = optionalPositinalCount;
                }


                size_t currentTokenPosition = 0;
                size_t optionalParsed = 0;

                for (auto& el : m_positionalArgumentNames)
                {
                    Argument& argument = m_arguments[el.positionInArguments];
                    argObj.Parse(argument, el.positionInArguments, "");

                    if (m_arguments[el.positionInArguments].m_required)
                    {

                        if (m_arguments[el.positionInArguments].m_nargs != kFromOneToInfinteArgCount)
                        {
                            for (size_t i = 0; i < m_arguments[el.positionInArguments].m_nargs; ++i)
                            {
                                if (!argObj.Parse(argument, currentArgumentObjectIndex, positionalArgs[currentTokenPosition]))
                                {
                                    return argObj;
                                }
                                ++currentTokenPosition;
                            }
                        }
                        else
                        {
                            size_t addtionalArg = 0;
                            if (additionalTokensForFirstRequiredNarg > 0)
                            {
                                ++addtionalArg;
                                --additionalTokensForFirstRequiredNarg;
                            }
                            for (size_t i = 0; i < totalTokensForRequiredNargs + addtionalArg; ++i)
                            {
                                if (!argObj.Parse(argument, currentArgumentObjectIndex, positionalArgs[currentTokenPosition]))
                                {
                                    return argObj;
                                }
                                ++currentTokenPosition;
                            }
                        }
                    }
                    else if (optionalParsed <= howMuchOptionalArgsCanBeParsed)
                    {
                        if (!argObj.Parse(argument, currentArgumentObjectIndex, positionalArgs[currentTokenPosition]))
                        {
                            return argObj;
                        }
                        ++currentTokenPosition;
                        ++optionalParsed;
                    }
                }
            }

            for (size_t i = 0; i < m_arguments.size(); ++i)
            {
                auto el = m_arguments[i];
                const std::string& name = el.m_longName.empty() ? (el.m_shortName.empty() ? el.m_positionalName : el.m_shortName) : el.m_longName;

                ArgumentParsed parsedArg = argObj.GetArg(name);

                if (parsedArg.GetArgumentExists())
                {
                    if (parsedArg.GetArgumentCount() == el.m_nargs
                        || el.m_nargs == kAnyArgCount
                        || (el.m_nargs == kFromOneToInfinteArgCount && parsedArg.GetArgumentCount() >= 1))
                    {
                        continue;
                    }
                    else
                    {
                        argObj.SetErrorString("Wrong arguments count for argument with name '" + name + "' got = " + std::to_string(parsedArg.GetArgumentCount()));
                        return argObj;
                    }
                }
                else if (el.HasDefault())
                {
                    argObj.ParseDefault(el, i);
                }
                else if (el.m_required)
                {
                    argObj.SetErrorString("Required argument with name '" + name + "' doesn't exists");
                    return argObj;
                }
            }

            argObj.SetValid();
            return argObj;
        }

        void _uknownArgumentHit(size_t& currentArgumentObjectIndex, bool& positionalArgsEndFlag, const std::string& el)
        {
            if (m_ignoreUknownArgs)
            {
                currentArgumentObjectIndex = kSizeTypeEnd;
                positionalArgsEndFlag = true;
            }
            else
            {
                throw std::runtime_error("Unknown input argument: " + el);
            }
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

        /// @brief Function to get help string
        /// @param width current terminla width (80 by default)
        /// @param nameWidthPercent percantage of current width, for naming parameters (30 by default)
        /// @return help string with proper new lines
        std::string GetHelp(size_t width = kHelpWidth, size_t nameWidthPercent = kHelpNameWidthPercent)
        {
            width = width < kHelpWidth ? kHelpWidth : width;
            size_t nameWidthInHelp = kHelpNameWidthPercent * width / 100;
            width -= nameWidthInHelp;


            std::stringstream usage("usage: ");
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

            if (!m_description.empty())
            {
                AddAdditionalDescription(usage, m_description, width);
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
                usage << "\n\n" << "optional arguments:\n\n";
                for (auto& el : m_arguments)
                {
                    if (el.m_positionalName.empty())
                    {
                        MakeDescriptionForArg(el, usage, nameWidthInHelp, width);
                    }
                }
            }

            if (!m_epilog.empty())
            {
                AddAdditionalDescription(usage, m_epilog, width);
            }

            return usage.str();
        }

    private:


        /// @brief Private function which is generate short names
        /// if m_allowAbbrev is true
        void AddShortNames()
        {
            std::string _pref{ m_prefix };

            for (auto& el1 : m_arguments)
            {
                if (!el1.m_longName.empty() && (!el1.m_positionalName.empty() || !el1.m_shortName.empty()))
                {
                    continue;
                }
                int count = 0;

                for (auto& el : m_knownArgumentNames)
                {
                    if (el.first.find(el1.m_longName.substr(0, 1)) != std::string::npos)
                    {
                        ++count;
                    }
                }
                if (count == 1)
                {
                    el1.m_shortName = el1.m_longName.substr(0, 1);
                    m_knownArgumentNamesInternal[_pref + el1.m_longName.substr(0, 1)] = { m_knownArgumentNames[el1.m_longName].position, KnownNameType::e_Short };
                }
            }
        }

        /// @brief Private function wich is called in case if AddArgument function
        /// succseed without errors
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
                    throw std::runtime_error("Short name '" + arg.m_shortName + "' already exists");
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
                    throw std::runtime_error("Long name '" + arg.m_longName + "' already exists");
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
                    throw std::runtime_error("Required positional argument with name '" + arg.m_positionalName + "' connot be with zero count");
                }
                else if (!arg.m_required && arg.m_nargs != 1)
                {
                    throw std::runtime_error("Non required positional argument with name '" + arg.m_positionalName + "' should be with count 1");
                }
                else
                {
                    throw std::runtime_error("Positional name " + arg.m_positionalName + " already exists");
                }
            }

            m_arguments.emplace_back(arg);
        }

        /// @brief Private function which generates usage according to all argument of program
        /// @param arg Argument instance
        /// @param usage out paramets, which return usage.
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
                usage << " [" << showName.str() << "[" << showName.str() << " ...]] ";
            }
            else if (arg.m_nargs == kFromOneToInfinteArgCount)
            {
                usage << " [" << showName.str() << " ...] ";
            }
            else if (arg.m_nargs != 0)
            {
                usage << " [";
                usage << showName.str();
                for (size_t i = 1; i < arg.m_nargs; ++i)
                {
                    usage << " " << showName.str();
                }
                usage << "] ";
            }
            if (!arg.m_required)
            {
                usage << "] ";
            }
        }

        /// @brief Private function which generates description for every Argument instance which added to parser
        /// This function called from GetHelp functions chain in case if m_addHelp is true.
        /// @param arg Argument instace
        /// @param description string stream to which add description
        /// @param nameLen actual lenght of names column in usage. If names are longer then nameLen new line will be added befor first line of description/
        /// nameLen is calculated accrding to terminal width and name nameWidthPercent from GetHelp input parameters
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
                showName << "," << m_prefix << m_prefix << arg.m_longName;
            }

            bool addSpace = static_cast<bool>(arg.m_help.size());

            showDesc << arg.m_help;
            if (arg.m_nargs)
            {
                showDesc << (addSpace ? " " : "") << "Type: " << m_enumToString[arg.m_type] << ". ";
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
            case kFromOneToInfinteArgCount:
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
            if (currentLen >= nameLen - 1)
            {
                description << "\n";
            }
            else
            {
                size_t spaceFillerSize = nameLen - currentLen;
                std::string filler;
                filler.resize(spaceFillerSize + 1);
                memset(&filler[0], ' ', spaceFillerSize);
                description << filler;
            }

            currentLen = 0;

            std::string filler;
            filler.resize(nameLen + 1);
            memset(&filler[0], ' ', nameLen);

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


        /// @brief Private function to add epilog or program description for programm help if
        /// autogenerated help is requested.
        /// @param description Current description stream
        /// @param additionalDesc Description to add
        /// @param descLen lenth of terminal
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


        /// @brief Private function wich generates choices for Argument intance description, if argument has
        /// any choisec
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
        bool        m_allowAbbrev = true;
        bool        m_addHelp = true;
        bool        m_ignoreUknownArgs = false;
        char        m_prefix = '-';

        std::string m_name;
        std::string m_description{ "" };
        std::string m_epilog{ "" };
        std::string m_usage{ "" };
        std::vector<Argument> m_arguments;
        std::vector<PositionalNamesStruct>      m_positionalArgumentNames;
        std::map<std::string, KnownNamesStruct> m_knownArgumentNames;
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