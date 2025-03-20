#ifndef XCMDLINE_PARSER_H
#define XCMDLINE_PARSER_H
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include <variant>
#include <string_view>

namespace xcmdline
{
    class parser
    {
    public:
        struct error : std::string { error()=default; constexpr error(std::string&& X) noexcept : std::string(std::move(X)){} };
        struct handle       { int m_Value=-1; std::strong_ordering operator <=>(const handle&) const noexcept       = default; };
        struct group_handle { int m_Value=-1; std::strong_ordering operator <=>(const group_handle&) const noexcept = default; };

        parser() = default;

    public:

        // Add groups useful when displaying the help message
        group_handle addGroup(std::string_view name, std::string_view description) noexcept
        {
            groups a;
            a.m_Name        = name;
            a.m_Description = description;
            
            m_Groups.push_back( std::move(a) );
            return { static_cast<int>(m_Groups.size() - 1) };
        }

        // Add an option with its properties
        handle addOption( std::string_view    flag
                        , std::string_view    description
                        , bool                required    = false
                        , int                 minArgs     = 0
                        , group_handle        group       = {} 
                        ) noexcept
        {
            Option opt;

            opt.m_Key           = std::hash<std::string_view>{}(flag);
            opt.m_Name          = flag;
            opt.m_Description   = description;
            opt.m_isRequired    = required;
            opt.m_minArgs       = static_cast<size_t>(minArgs);

            m_Options.push_back( std::move(opt) );

            const handle OptHandle = { static_cast<int>(m_Options.size() - 1) };
            if (group.m_Value >= 0 && group.m_Value < m_Groups.size())
            {
                m_Groups[group.m_Value].m_Options.push_back(OptHandle);
            }

            return OptHandle;
        }

        // Parse command line arguments, returns empty string on success or error message
        error Parse(int argc, const char* const argv[]) noexcept
        {
            if (argc > 0) 
            {
                m_programName = argv[0];
            }

            for (int i = 1; i < argc; ++i) 
            {
                std::string_view arg = argv[i];

                if (isFlag(arg)) 
                {
                    std::string flag = std::string(arg);
                    if (flag.substr(0, 2) == "--") 
                    {
                        flag = flag.substr(2);
                    }
                    else 
                    {
                        flag = flag.substr(1);
                    }

                    if (auto E = findOption(flag); std::holds_alternative<error>(E))
                    {
                        return std::get<error>(E);
                    }
                    else
                    {
                        const Option& opt = m_Options[std::get<handle>(E).m_Value];

                        std::vector<std::string>& args = opt.m_Args;
                        while (i + 1 < argc && !isFlag(argv[i + 1])) 
                        {
                            args.push_back(parseArgument(argv[++i], i, argc, argv));
                        }

                        if (args.size() < opt.m_minArgs)
                        {
                            return std::format( "Option - {} requires at least {} arguments", flag, std::to_string(opt.m_minArgs) );
                        }
                    }
                }
            }

            for (const auto& opt : m_Options) 
            {
                if (opt.m_isRequired && opt.m_Args.empty())
                {
                    return std::format( "Required option - {} is missing", opt.m_Name);
                }
            }

            return {};
        }

        void clear()
        {
            m_Options.clear();
            m_Groups.clear();
            m_programName.clear();
        }

        void clearArgs()
        {
            for (auto& E : m_Options) E.m_Args.clear();
        }

        // Parse command line arguments from a single string
        error Parse(std::string_view commandLine) noexcept
        {
            std::vector<std::string>    args;
            constexpr std::string_view  delimiters = " \t";

            size_t start = commandLine.find_first_not_of(delimiters);

            while (start != std::string_view::npos) 
            {
                size_t end = commandLine.find_first_of(delimiters, start);
                if (end == std::string_view::npos) 
                {
                    end = commandLine.length();
                }
                args.emplace_back(commandLine.substr(start, end - start));
                start = commandLine.find_first_not_of(delimiters, end);
            }

            std::vector<const char*> c_args;
            for (const auto& arg : args) 
            {
                c_args.push_back(arg.c_str());
            }

            return Parse(static_cast<int>(c_args.size()), c_args.data());
        }

        // Check if an option exists
        bool hasOption(handle hOption) const noexcept
        {
            const Option& opt = m_Options[hOption.m_Value];
            return !opt.m_Args.empty();
        }

        // Get number of arguments for an option
        size_t getOptionArgCount(handle hOption) const noexcept
        {
            const Option& opt = m_Options[hOption.m_Value];
            return opt.m_Args.size();
        }

        // Get option argument by index with type conversion
        template<typename T>
        std::variant<T, error> getOptionArgAs(handle hOption, size_t index = 0) const noexcept
        {
            const Option& opt = m_Options[hOption.m_Value];
            if (index >= opt.m_Args.size())
            {
                return error(std::format("Option - {} does not have argument {}", opt.m_Name, index));
            }

            return convertValue<T>(opt.m_Args[index], opt.m_Name, index);
        }

        // print help message with all the options
        void printHelp() const noexcept
        {
            std::cout << "Usage: " << m_programName << " [options] [positional arguments]\n";
            std::cout << "Options:\n";

            const int leftColumnWidth = 30;
            const std::string leftPadding(leftColumnWidth, ' ');

            // Helper lambda to print an option
            auto printOption = [&](const Option& opt, const std::string_view prefix) noexcept
            {
                std::ostringstream optStream;
                optStream << prefix << "-" << opt.m_Name;
                if (opt.m_Name.length() > 1)
                {
                    optStream << ", --" << opt.m_Name;
                }
                if (opt.m_minArgs > 0)
                {
                    optStream << " <args>";
                }
                std::string optText = optStream.str();
                if (optText.length() < leftColumnWidth)
                {
                    optText += std::string(leftColumnWidth - optText.length(), ' ');
                }
                else if (optText.length() > leftColumnWidth)
                {
                    optText = optText.substr(0, leftColumnWidth);
                }

                std::string desc ( opt.m_Description );
                if (opt.m_isRequired)
                {
                    desc += " (required)";
                }
                if (opt.m_minArgs > 0)
                {
                    desc += " (min " + std::to_string(opt.m_minArgs) + " args)";
                }

                size_t start = 0;
                bool firstLine = true;
                while (true)
                {
                    size_t pos = desc.find('\n', start);
                    std::string line = (pos == std::string::npos) ? desc.substr(start) : desc.substr(start, pos - start);
                    if (firstLine)
                    {
                        std::cout << optText << line << "\n";
                        firstLine = false;
                    }
                    else if (!line.empty())
                    {
                        std::cout << leftPadding << line << "\n";
                    }
                    if (pos == std::string::npos) break;
                    start = pos + 1;
                }
            };

            // Print grouped options
            for (const auto& group : m_Groups)
            {
                std::cout << "\n" << group.m_Name << ":\n";
                std::cout << "  " << group.m_Description << "\n";
                for (const auto& optHandle : group.m_Options)
                {
                    printOption(m_Options[optHandle.m_Value], "    ");
                }
            }

            // Print ungrouped options
            bool hasUngroupedOptions = false;
            for (size_t i = 0; i < m_Options.size(); ++i)
            {
                bool isGrouped = false;
                for (const auto& group : m_Groups)
                {
                    if (std::find(group.m_Options.begin(), group.m_Options.end(), handle{ static_cast<int>(i) }) != group.m_Options.end())
                    {
                        isGrouped = true;
                        break;
                    }
                }
                if (!isGrouped)
                {
                    if (!hasUngroupedOptions)
                    {
                        std::cout << "\nUngrouped Options:\n";
                        hasUngroupedOptions = true;
                    }
                    printOption(m_Options[i], "  ");
                }
            }
        }

    protected:

        struct Option
        {
            std::size_t                         m_Key;
            std::string                         m_Name;
            std::string                         m_Description;
            bool                                m_isRequired;
            size_t                              m_minArgs;
            mutable std::vector<std::string>    m_Args;
        };

        struct groups
        {
            groups() = default;
            groups(groups&&) = default;
            std::string                         m_Name          {};
            std::string                         m_Description   {};
            std::vector<handle>                 m_Options       {};
        };

        std::vector<Option>         m_Options;         // All options and their arguments
        std::vector<groups>         m_Groups;          // All groups and their options
        std::string                 m_programName;     // Name of the program

        // Helper function to check if a string is a flag
        bool isFlag(std::string_view arg) const
        {
            return !arg.empty() && (arg[0] == '-' || arg.substr(0, 2) == "--");
        }

        // Parse a single quoted or unquoted argument
        std::string parseArgument(std::string_view arg, int& i, int argc, const char* const argv[])
        {
            if (arg.front() == '"' && arg.back() != '"') 
            {
                std::ostringstream oss;
                oss << arg.substr(1);
                while (i + 1 < argc) 
                {
                    std::string_view nextArg = argv[++i];
                    if (nextArg.back() == '"') 
                    {
                        oss << " " << nextArg.substr(0, nextArg.length() - 1);
                        break;
                    }
                    oss << " " << nextArg;
                }
                return oss.str();
            }
            return std::string(arg);
        }

        // Find option by flag (linear search) hash key helps a bit...
        std::variant<handle,error> findOption(std::string_view flag)
        {
            std::size_t Key = std::hash<std::string_view>{}(flag);

            int index = 0;
            for (auto& opt : m_Options) 
            {
                if (opt.m_Key == Key && opt.m_Name == flag)
                {
                    return handle{ index };
                }
                ++index;
            }

            return std::format("Unknown option - {}", flag);
        }

        // const version of the find option function
        std::variant<handle, error> findOption(std::string_view flag) const
        {
            return const_cast<parser*>(this)->findOption(flag);
        }

        // Convert string to specified type, returning value or error message
        template<typename T>
        auto convertValue(const std::string& value, std::string_view flag, size_t argIndex) const;
    };

    // Template specializations for type conversion
    template<>
    auto parser::convertValue<std::string>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        return value;
    }

    template<>
    auto parser::convertValue<std::string_view>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        return std::string_view(value);
    }

    template<>
    auto parser::convertValue<int64_t>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        std::istringstream iss(value);
        int64_t result;
        iss >> result;
        if (iss.fail() || !iss.eof()) 
        {
            return std::variant<int64_t, error>{"Failed to convert '" + value + "' to integer at argument " + std::to_string(argIndex) + " of -" + std::string(flag)};
        }
        return std::variant<int64_t, error>{result};
    }

    template<>
    auto parser::convertValue<double>(const std::string& value, std::string_view flag, size_t argIndex) const {
        std::istringstream iss(value);
        double result;
        iss >> result;
        if (iss.fail() || !iss.eof()) 
        {
            return std::variant<double, error>{"Failed to convert '" + value + "' to double at argument " + std::to_string(argIndex) + " of -" + std::string(flag)};
        }
        return std::variant<double, error>{result};
    }

} // namespace xcmdline

#endif // XCMDLINE_PARSER_H