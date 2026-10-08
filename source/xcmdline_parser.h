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
#include <cctype>
#include "source/xerr.h"

namespace xcmdline
{
    enum state : std::uint8_t
    { OK
    , FAILURE
    };

    class parser
    {
    public:
        struct handle       { int m_Value=-1; std::strong_ordering operator <=>(const handle&) const noexcept       = default; };
        struct group_handle { constexpr group_handle(int V = -1) noexcept : m_Value(V) {} int m_Value; std::strong_ordering operator <=>(const group_handle&) const noexcept = default; };

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

        // One word of a command line. A quoted word was written with quotes ("-5", "-Name"): whatever is inside is a value, never a flag.
        struct token { std::string m_Text; bool m_bQuoted = false; };

        // Splits a command line the way Windows does (CommandLineToArgvW), so that a person, a script and the OS agree and a path needs no care:
        //   - words are separated by spaces, tabs and line breaks, except inside "quotes", where all of them are kept as they are;
        //   - a backslash is just a backslash, unless it comes right before a quote: 2n backslashes + a quote = n backslashes and the quote opens or closes;
        //     2n+1 backslashes + a quote = n backslashes and a literal quote ("C:\dir\" is C:\dir\ ; say \" for a quote inside a value);
        //   - inside quotes, "" is a literal quote;
        //   - "" on its own is an empty value.
        // pUnterminated tells that the line ended inside a quote (a pipe waits for the rest of it: a value may hold line breaks).
        static std::vector<token> Tokenize(std::string_view Line, bool* pUnterminated = nullptr) noexcept
        {
            std::vector<token> Out;
            std::string        Cur;
            bool               bIn = false, bHave = false, bQuoted = false;
            for (std::size_t i = 0; i < Line.size(); ++i)
            {
                const char c = Line[i];
                if (c == '\\')
                {
                    std::size_t n = 0;
                    while (i < Line.size() && Line[i] == '\\') { ++n; ++i; }
                    if (i < Line.size() && Line[i] == '"')
                    {
                        Cur.append(n / 2, '\\');
                        if (n % 2) Cur += '"'; else --i;                 // an even run leaves the quote for the next turn, where it opens or closes
                    }
                    else { Cur.append(n, '\\'); --i; }
                    bHave = true;
                    continue;
                }
                if (c == '"')
                {
                    if (bIn && i + 1 < Line.size() && Line[i + 1] == '"') { Cur += '"'; ++i; bHave = true; continue; }
                    bIn = !bIn; bHave = true; bQuoted = true;
                    continue;
                }
                if (!bIn && (c == ' ' || c == '\t' || c == '\r' || c == '\n'))
                {
                    if (bHave) { Out.push_back({ std::move(Cur), bQuoted }); Cur.clear(); bHave = bQuoted = false; }
                    continue;
                }
                Cur += c; bHave = true;
            }
            if (bHave) Out.push_back({ std::move(Cur), bQuoted });
            if (pUnterminated) *pUnterminated = bIn;
            return Out;
        }

        // The other way: any text as ONE value of a command line (what Tokenize reads back as exactly this text). Always in quotes, so it may hold spaces, tabs, line breaks, a
        // leading '-' or nothing at all; a quote is written \" and the backslashes in front of a quote or of the closing quote are doubled.
        static std::string Quote(std::string_view Text) noexcept
        {
            std::string Out = "\"";
            std::size_t nBackslashes = 0;
            for (const char c : Text)
            {
                if (c == '\\') { ++nBackslashes; continue; }
                if (c == '"') { Out.append(nBackslashes * 2 + 1, '\\'); Out += '"'; }
                else          { Out.append(nBackslashes, '\\'); Out += c; }
                nBackslashes = 0;
            }
            Out.append(nBackslashes * 2, '\\');
            Out += '"';
            return Out;
        }

        // Parse command line arguments, returns empty string on success or error message
        xerr Parse(int argc, const char* const argv[]) noexcept
        {
            if (argc > 0)
            {
                m_programName = argv[0];
            }

            // The words of a real argv were already split at the spaces by whoever built it: a quoted value that held spaces is put back together here
            std::vector<token> Tokens;
            for (int i = 1; i < argc; ++i)
            {
                const std::string_view arg = argv[i];
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') { Tokens.push_back({ std::string(arg.substr(1, arg.size() - 2)), true }); continue; }
                if (!arg.empty() && arg.front() == '"')
                {
                    std::string Joined(arg.substr(1));
                    while (i + 1 < argc)
                    {
                        std::string_view next = argv[++i];
                        if (!next.empty() && next.back() == '"') { Joined += ' '; Joined.append(next.substr(0, next.size() - 1)); break; }
                        Joined += ' '; Joined.append(next);
                    }
                    Tokens.push_back({ std::move(Joined), true });
                    continue;
                }
                Tokens.push_back({ std::string(arg), false });
            }
            return ParseTokens(Tokens);
        }

        xerr ParseTokens(const std::vector<token>& Tokens) noexcept
        {
            const auto IsFlagToken = [&](const token& T) { return !T.m_bQuoted && isFlag(T.m_Text); };
            for (std::size_t i = 0; i < Tokens.size(); ++i)
            {
                if (!IsFlagToken(Tokens[i])) continue;
                std::string flag = Tokens[i].m_Text;
                flag = flag.substr(flag.substr(0, 2) == "--" ? 2 : 1);

                const auto E = findOption(flag);
                if (std::holds_alternative<xerr>(E))
                {
                    return std::get<xerr>(E);
                }
                const Option& opt = m_Options[std::get<handle>(E).m_Value];
                std::vector<std::string>& args = opt.m_Args;
                while (i + 1 < Tokens.size() && !IsFlagToken(Tokens[i + 1])) args.push_back(Tokens[++i].m_Text);

                if (args.size() < opt.m_minArgs)
                {
                    xerr::LogMessage<state::FAILURE>(std::format("Option - {} requires at least {} arguments", flag, std::to_string(opt.m_minArgs)));
                    return xerr::create_f<state, "Missing arguments">();
                }
            }

            for (const auto& opt : m_Options)
            {
                if (opt.m_isRequired && opt.m_Args.empty())
                {
                    xerr::LogMessage<state::FAILURE>(std::format("Required option - {} is missing", opt.m_Name));
                    return xerr::create_f<state, "A required option is missing">();
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

        // Parse command line arguments from a single string (see Tokenize for the rules: quotes, backslashes, line breaks inside a quoted value)
        xerr Parse(std::string_view commandLine) noexcept
        {
            return ParseTokens(Tokenize(commandLine));
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
        std::variant<T, xerr> getOptionArgAs(handle hOption, size_t index = 0) const noexcept
        {
            const Option& opt = m_Options[hOption.m_Value];
            if (index >= opt.m_Args.size())
            {
                xerr::LogMessage<state::FAILURE>(std::format("Option - {} does not have argument {}", opt.m_Name, index));
                return xerr::create_f<state, "Option is missing arguments">();
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

        // Helper function to check if a string is a flag - a leading '-' alone isn't enough: a bare
        // "-" (used as an "empty value" placeholder by some callers) and a negative number like "-1"
        // both start with '-' too but are values, not flag names.
        bool isFlag(std::string_view arg) const
        {
            if (arg.empty() || arg[0] != '-' || arg.size() == 1) return false;
            if (std::isdigit(static_cast<unsigned char>(arg[1]))) return false;
            return true;
        }

        // Parse a single quoted or unquoted argument
        std::string parseArgument(std::string_view arg, int& i, int argc, const char* const argv[])
        {
            // A quoted value with no internal space/tab is already a single token here (e.g. "C:\a\b")
            // - front() and back() are BOTH '"' in the same token, so this must be checked before the
            // "spans multiple tokens" case below, or the surrounding quote characters never get
            // stripped and end up as literal characters in the returned value.
            if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
                return std::string(arg.substr(1, arg.size() - 2));
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
        std::variant<handle,xerr> findOption(std::string_view flag)
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

            xerr::LogMessage<state::FAILURE>(std::format("Unknown option - {}", flag));
            return xerr::create_f<state, "Unknown option">();
        }

        // const version of the find option function
        std::variant<handle, xerr> findOption(std::string_view flag) const
        {
            return const_cast<parser*>(this)->findOption(flag);
        }

        // Convert string to specified type, returning value or error message
        template<typename T>
        auto convertValue(const std::string& value, std::string_view flag, size_t argIndex) const;
    };

    // Template specializations for type conversion - inline on every one: an EXPLICIT (full)
    // specialization of a function template is an ordinary function by the time it's instantiated,
    // not automatically inline/weak-linkage the way an implicitly-instantiated template is, so
    // without this keyword a header defining these has external linkage and violates ODR the moment
    // more than one .cpp in the same link includes it. Confirmed live: harmless while only one
    // xGPU example used xundo (which pulls this header in transitively), a real multiply-defined-
    // symbol link error the moment a second one (E29) did too.
    template<>
    inline auto parser::convertValue<std::string>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        return value;
    }

    template<>
    inline auto parser::convertValue<std::string_view>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        return std::string_view(value);
    }

    template<>
    inline auto parser::convertValue<int64_t>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        std::istringstream iss(value);
        int64_t result;
        iss >> result;
        if (iss.fail() || !iss.eof())
        {
            xerr::LogMessage<state::FAILURE>("Failed to convert '" + value + "' to integer at argument " + std::to_string(argIndex) + " of -" + std::string(flag) );
            return std::variant<int64_t, xerr>{xerr::create_f<state, "conversion failure from string to float">()};
        }
        return std::variant<int64_t, xerr>{result};
    }

    template<>
    inline auto parser::convertValue<double>(const std::string& value, std::string_view flag, size_t argIndex) const
    {
        std::istringstream iss(value);
        double result;
        iss >> result;
        if (iss.fail() || !iss.eof())
        {
            xerr::LogMessage<state::FAILURE>("Failed to convert '" + value + "' to double at argument " + std::to_string(argIndex) + " of -" + std::string(flag));
            return std::variant<double, xerr>{xerr::create_f<state,"conversion failure from string to double">()};
        }
        return std::variant<double, xerr>{result};
    }

} // namespace xcmdline

#endif // XCMDLINE_PARSER_H