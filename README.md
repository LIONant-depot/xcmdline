# xcmdline: Supercharge Your C++ Command-Line Parsing!

Effortlessly parse command-line arguments with **xcmdline** – a modern, lightweight C++20 library designed for robust and flexible command-line option handling. With an intuitive API, support for grouped options, type-safe argument parsing, and detailed help message generation, xcmdline simplifies command-line processing for any C++ project. Boost productivity and focus on your application logic, not argument parsing!

## Key Features

- **Intuitive Option Management**: Easily add and group options with descriptions for organized parsing.
- **Type-Safe Argument Retrieval**: Convert arguments to `int64_t`, `double`, `std::string`, or `std::string_view` with error handling.
- **Flexible Parsing**: Supports both `argc/argv` and single-string command-line inputs.
- **Required Option Enforcement**: Ensure critical options are provided with customizable argument counts.
- **Grouped Help Messages**: Automatically generate well-formatted help output with grouped options.
- **Error Handling**: Robust error reporting via `xerr` for invalid options or missing arguments.
- **C++20 Modern Design**: Leverages modern C++20 features for a clean, safe, and efficient API.
- **Minimal Dependencies**: Requires only standard C++20 libraries and `xerr.h` for error handling.
- **MIT License**: Free and open for all your projects.
- **Easy Integration**: Simply include `xcmdline_parser.h` in your project.

## Dependencies

- Standard C++20 libraries
- `xerr.h` (custom error handling utility)

## Getting Started

1. **Add to Your Project**:
   - Include `source/xcmdline_parser.h` in your project.
   - Ensure `xerr.h` is available for error handling.

## Code Example

```cpp
#include "source/xcmdline_parser.h"
#include <cassert>

int main(int argc, char* argv[]) {
    xcmdline::parser parser;

    // Define option groups
    auto mainGroup = parser.addGroup("Main Options", "Required options for the program");
    auto optGroup = parser.addGroup("Optional Settings", "Customize program behavior");

    // Add options
    auto hProject = parser.addOption("project", "Path to project folder", true, 1, mainGroup);
    auto hVerbose = parser.addOption("verbose", "Enable verbose output", false, 0, optGroup);
    auto hLevel = parser.addOption("level", "Optimization level (0-2)", false, 1, optGroup);

    // Parse command-line arguments
    if (auto err = parser.Parse(argc, argv); err) {
        parser.printHelp();
        return 1;
    }

    // Check for help request
    if (parser.hasOption(hVerbose)) {
        std::cout << "Verbose mode enabled\n";
    }

    // Retrieve and validate project path
    if (auto result = parser.getOptionArgAs<std::string>(hProject, 0); std::holds_alternative<xerr>(result)) {
        std::cerr << "Error: " << std::get<xerr>(result).message() << "\n";
        return 1;
    } else {
        std::cout << "Project path: " << std::get<std::string>(result) << "\n";
    }

    // Retrieve optimization level
    if (parser.hasOption(hLevel)) {
        if (auto result = parser.getOptionArgAs<int64_t>(hLevel, 0); std::holds_alternative<xerr>(result)) {
            std::cerr << "Error: " << std::get<xerr>(result).message() << "\n";
            return 1;
        } else {
            std::cout << "Optimization level: " << std::get<int64_t>(result) << "\n";
        }
    }

    return 0;
}
```

## Usage Notes

- Use `addGroup` to organize related options for clearer help messages.
- Specify `required` and `minArgs` in `addOption` to enforce option presence and argument counts.
- Use `getOptionArgAs<T>` to retrieve arguments with type conversion and error checking.
- Call `printHelp()` to display a formatted help message with grouped and ungrouped options.

Dive into xcmdline today – star, fork, and contribute to supercharge your string manipulation! 🚀