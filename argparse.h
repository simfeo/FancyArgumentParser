#pragma once

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

namespace ARGPARSE_NAMESPACE_NAME
{
    namespace
    {
        const int kAnyArgCount = -1;
        const int kFromOneToInfinteArgCount = -2;
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

    enum class ArgTypeCast : int
    {
        e_String,
        e_int,
        e_longlong,
        e_double,
        e_bool
    };


    class ArgumentParser;
    class ArgumentsObject;

    class Argument
    {
    public:
        Argument(const std::string& positionalName = "",
            const std::string& shortName = "",
            const std::string& longName = "",
            const int argsCount = 1,
            ArgTypeCast argType = ArgTypeCast::e_String,
            const bool required = true,
            const std::string& help = "",
            const std::vector<std::string> choices = {})
            : m_required(required)
            , m_nargs(argsCount)
            , m_type(argType)
            , m_positionalName(positionalName)
            , m_shortName(shortName)
            , m_longName(longName)
            , m_help(help)
        {}
        // is argument optional or required
        bool m_required = true;

        Argument& SetRequired(bool required)
        {
            m_required = required;
            return *this;
        }

        // argument ammount
        // -1 ammount is any
        // -2 ammount is from 1 to infinite
        // 0 ammout is a for flag argument
        int m_nargs = 1;

        Argument& SetNumberOfArguments(int ammount)
        {
            m_nargs = ammount;
            return *this;
        }

        Argument& SetAnyNumberOfArgumentsButAtleastOne()
        {
            m_nargs = kFromOneToInfinteArgCount;
            return *this;
        }

        Argument& SetAnyNumberOfArguments()
        {
            m_nargs = kAnyArgCount;
            return *this;
        }

        Argument& SetArgumentIsFlag()
        {
            m_nargs = 0;
            return *this;
        }

        // types of argument
        ArgTypeCast m_type = ArgTypeCast::e_String;

        Argument& SetType(ArgTypeCast argType)
        {
            m_type = argType;
            return *this;
        }

        // positional argument name
        // positional argument starts without any prefixe
        std::string m_positionalName = "";

        Argument& SetPositionalName(const std::string& name)
        {
            m_positionalName = name;
            return *this;
        }

        // argument short name
        // for non positional argument only
        // shot name used in input with 1 prefix
        std::string m_shortName = "";

        Argument& SetShortName(const std::string& name)
        {
            m_shortName = name;
            return *this;
        }

        // argument long name
        // for non positional arguments only
        // agument long name start with double prefix
        std::string m_longName = "";

        Argument& SetLongName(const std::string& name)
        {
            m_longName = name;
            return *this;
        }

        // help info
        std::string m_help = "";

        Argument& SetHelp(const std::string& help)
        {
            m_help = help;
            return *this;
        }

        // allowed values for argument
        // will be values will 
        std::vector<std::string> m_choicesString = {};

        Argument& SetChoices(const std::vector<std::string>& choices)
        {
            if (m_type != ArgTypeCast::e_String)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesString = choices;
            return *this;
        }

        // allowed values for argument
        // will be values will 
        std::vector<int> m_choicesInt = {};

        Argument& SetChoices(const std::vector<int>& choices)
        {
            if (m_type != ArgTypeCast::e_int)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesInt = choices;
            return *this;
        }

        // allowed values for argument
        // will be values will 
        std::vector<long long> m_choicesLongLong = {};

        Argument& SetChoices(const std::vector<long long>& choices)
        {
            if (m_type != ArgTypeCast::e_longlong)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesLongLong = choices;
            return *this;
        }

        // allowed values for argument
        // will be values will 
        std::vector<double> m_choicesDouble = {};

        Argument& SetChoices(const std::vector<double>& choices)
        {
            if (m_type != ArgTypeCast::e_double)
            {
                throw std::runtime_error("wrong type");
            }
            m_choicesDouble = choices;
            return *this;
        }

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

    class ArgumentParsed
    {
    public:
        bool GetArgumentExists()
        {
            return m_exists;
        }

        size_t GetArgumentCount()
        {
            return m_count;
        }

        const bool GetAsBool() const
        {
            return m_bool.front();
        }
        const int GetAsInt() const
        {
            return m_int.front();
        }
        const long long GetAsLongLong() const
        {
            return m_longLong.front();
        }
        const double GetAsDouble() const
        {
            return m_double.front();
        }
        const std::string& GetAsString() const
        {
            return m_string.front();
        }

        const std::vector<bool>& GetAsVecBool() const
        {
            return m_bool;
        }

        const std::vector<int>& GetAsVecInt() const
        {
            return m_int;
        }

        const std::vector<long long>& GetAsVecLongLong() const
        {
            return m_longLong;
        }

        const std::vector<double>& GetAsVecDouble() const
        {
            return m_double;
        }

        const std::vector<std::string>& GetAsVecString() const
        {
            return m_string;
        }

#if __cplusplus > 201402L || _MSVC_LANG > 201402L
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

    class ArgumentsObject
    {
    public:
        bool IsArgValid() const
        {
            return m_isValid;
        }

        const std::string& GetErrorString()
        {
            return m_error;
        }

        const size_t ParsedArgsCount() const
        {
            return m_parsed.size();
        }

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

        bool InvalidateArgsOutOfChoice(const argparse::Argument& argObj, const std::string& token)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("Value '" + token + "' is out of choices for" + name);

            return false;
        }

        bool InvalidateArgsTooMany(const argparse::Argument& argObj)
        {
            const std::string& name = argObj.m_longName.empty() ? (argObj.m_shortName.empty() ? argObj.m_positionalName : argObj.m_shortName) : argObj.m_longName;

            SetErrorString("too many argument for " + name);

            return false;
        }

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


    void NewFunction(std::stringstream& showDesc)
    {
        showDesc.seekg(0, std::ios::end);
        size_t size = showDesc.tellg();
        showDesc.seekg(0, std::ios::beg);
    }

    class ArgumentParser
    {
    public:
        ArgumentParser(const std::string& name) noexcept
            : m_name(name)
        {}

        // Text to display before the argument help
        // (default: "")
        ArgumentParser& SetDescription(const std::string& description) noexcept
        {
            m_description = description;
            return *this;
        }

        // Allows long options to be abbreviated if the abbreviation is unambiguous.
        // (default: True)
        ArgumentParser& SetAllowAbbrev(bool allowAbbrev) noexcept
        {
            m_allowAbbrev = allowAbbrev;
            return *this;
        }

        // Allows ignore unknown named arguments
        // (default: false)
        ArgumentParser& SetIgnoreUknownArgs(bool ignoreUnknownArgs) noexcept
        {
            m_ignoreUknownArgs = ignoreUnknownArgs;
            return *this;
        }

        // Add a - h / --help option to the parser
        // (default: True)
        ArgumentParser& SetAddHelp(bool addHelp) noexcept
        {
            m_addHelp = addHelp;
            return *this;
        }

        // Text to display after the argument help
        // (default: "")
        ArgumentParser& SetEpilog(const std::string& epilog) noexcept
        {
            m_epilog = epilog;
            return *this;
        }

        // The string describing the program usage
        // (default: generated from arguments added to parser)
        ArgumentParser& SetUsage(const std::string& usage) noexcept
        {
            m_usage = usage;
            return *this;
        }

        // The set of characters that prefix optional arguments
        // (default: '-')
        ArgumentParser& SetPrefixChars(const char charSym) noexcept
        {
            m_prefix = charSym;
            return *this;
        }


        // function to add aruments specification to command line parser
        void AddArgument(const Argument& arg)
        {
            if (arg.m_longName.empty() && arg.m_shortName.empty() && arg.m_positionalName.empty())
            {
                throw std::runtime_error("Short,long or positional names of argument are empty.\n"
                    "At least one name should been specified.");
            }
            else if (!(arg.m_longName.empty() || arg.m_shortName.empty()) && !arg.m_positionalName.empty())
            {
                throw std::runtime_error("Positiona argument " + arg.m_positionalName + " decalred aside with short/long name\n"
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

        ArgumentsObject ParseArgs(const std::vector<std::string>& args)
        {
            std::string _pref{ m_prefix };
            std::string _doublePref{ m_prefix, m_prefix };

            for (auto& el : m_knownArgumentNames)
            {
                if (el.second.argNameType == KnowNameType::e_Short)
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
                    Argument arg;
                    if (!shortHelpAlreadyExists)
                    {
                        arg.m_shortName = "h";
                    }
                    if (!longHelpAlreadyExists)
                    {
                        arg.m_longName = "help";
                    }
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

        ArgumentsObject ParseArgs(const int argc, char** argv)
        {

            std::vector<std::string> args;
            for (int i = 1; i < argc; ++i)
            {
                args.emplace_back(argv[i]);
            }

            return ParseArgs(args);
        }

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
                    m_knownArgumentNamesInternal[_pref + el1.m_longName.substr(0, 1)] = { m_knownArgumentNames[el1.m_longName].position, KnowNameType::e_Short };
                }
            }
        }

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




        //formatter_class - A class for customizing the help output
        //conflict_handler - The strategy for resolving conflicting optionals(usually unnecessary)
    private:

        void _addArg(const Argument& arg)
        {
            size_t currentSize = m_arguments.size();
            if (!arg.m_shortName.empty())
            {
                if (m_knownArgumentNames.find(arg.m_shortName) == m_knownArgumentNames.end())
                {
                    m_knownArgumentNames[arg.m_shortName] = { currentSize, KnowNameType::e_Short };
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
                    m_knownArgumentNames[arg.m_longName] = { currentSize, KnowNameType::e_Long };
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
                    throw std::runtime_error("Non required positiona argument with name '" + arg.m_positionalName + "' should be with count 1");
                }
                else
                {
                    throw std::runtime_error("Positional name " + arg.m_positionalName + " already exists");
                }
            }

            m_arguments.emplace_back(arg);
        }

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
            showDesc << (addSpace ? " " : "") << "Type: " << m_enumToString[arg.m_type] << ". ";

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

            showDesc << "Args count: ";
            switch (arg.m_nargs)
            {
            case kAnyArgCount:
                showDesc << "any. ";
                break;
            case kFromOneToInfinteArgCount:
                showDesc << " at least one. ";
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


        void AddAdditionalDescription(std::stringstream& description, const std::string& additionADesc, size_t descLen)
        {
            description << "\n";
            size_t currentLen = 0;
            std::stringstream buffer{ additionADesc };
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


        template<typename T>
        void MakeChoicesToString(std::stringstream& showName, std::vector<T>& choices)
        {
            showName << choices.front();
            for (size_t i = 1; i < choices.size(); ++i)
            {
                showName << ", " << choices[i];
            }
        }

    private:
        enum class KnowNameType :int
        {
            e_Short,
            e_Long
        };
        struct KnownNamesStruct
        {
            size_t position;
            KnowNameType argNameType;
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