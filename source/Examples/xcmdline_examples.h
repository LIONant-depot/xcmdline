#ifndef XCMDLINE_EXAMPLES_H
#define XCMDLINE_EXAMPLES_H
#pragma once

#include "../xcmdline_parser.h"

namespace xcmdline
{
    inline
    int Example(void)
    {
        const char* argv[] =
        { "ProgramExample"
        , "--input"
        , "data.txt"
        , "--output"
        , "result.txt"
        , "-v"
        , "--scale"
        , "2.5"
        };
        int argc = sizeof(argv) / sizeof(argv[0]);

        xcmdline::parser parser;

        // Define groups
        const auto gInput   = parser.addGroup("Input Options", "Options for input handling");
        const auto gOutput  = parser.addGroup("Output Options", "Options for output configuration");

        // Define options with embedded newlines in descriptions
        const auto hHelp    = parser.addOption("h", "Show this help message\nUse -h or --h to display", false, 0, gInput);
        const auto hInput   = parser.addOption("input", "Input file path\nSpecify a valid file or use - for stdin\nDefault is stdin if omitted", true, 1, gInput);
        const auto hOutput  = parser.addOption("output", "Output file path\nLong description split\ninto multiple lines", false, 1, gOutput);
        const auto hVerbose = parser.addOption("v", "Enable verbose logging\nSet to see detailed output", false, 0, gOutput);
        const auto hScale   = parser.addOption("scale", "Scale factor for processing\nValues between 0.1 and 10.0\nDefault is 1.0", false, 1); // Ungrouped


        // Parse command line arguments
        std::string parseError = parser.Parse(argc, argv);

        // Just just print the help message and check it out...
        parser.printHelp();


        if (!parseError.empty())
        {
            std::cerr << "Error: " << parseError << "\n";
            parser.printHelp();
            return 1;
        }

        // Check for help flag
        if (parser.hasOption(hHelp))
        {
            parser.printHelp();
            return 0;
        }

        // Process input option (required)
        if (parser.hasOption(hInput))
        {
            auto result = parser.getOptionArgAs<std::string>(hInput, 0);
            if (std::holds_alternative<std::string>(result))
            {
                std::cout << "Input file: " << std::get<std::string>(result) << "\n";
            }
            else
            {
                std::cerr << "Error: " << std::get<xcmdline::parser::error>(result) << "\n";
            }
        }

        // Process output option (optional)
        if (parser.hasOption(hOutput))
        {
            auto result = parser.getOptionArgAs<std::string>(hOutput, 0);
            if (std::holds_alternative<std::string>(result))
            {
                std::cout << "Output file: " << std::get<std::string>(result) << "\n";
            }
            else
            {
                std::cerr << "Error: " << std::get<xcmdline::parser::error>(result) << "\n";
            }
        }

        return 0;
    }
}

#endif // XCMDLINE_EXAMPLES_H