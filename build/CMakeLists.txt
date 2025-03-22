# dependencies/xcmdline/build/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Define xcmdline as an interface library only if not already defined
if(NOT TARGET xcmdline)
  add_library(xcmdline INTERFACE)
  target_include_directories(xcmdline INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../source")
endif()
