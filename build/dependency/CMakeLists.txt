include(${CMAKE_BINARY_DIR}/_deps/xcmake_tools/Common.cmake)

# Fetch dependencies
FetchAndPopulate("https://github.com/LIONant-depot/xerr.git"    "main")


DefineInterfaceComponent(xcmdline "dependencies/xcore"
  "source/xcmdline_parser.h"
  "readme.md"
)
