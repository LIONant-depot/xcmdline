cmake_minimum_required(VERSION 3.10)

# Define as an interface library
if(NOT TARGET xcmdline)
    add_library(xcmdline INTERFACE)
   
    #set the root for this project
    set(ROOT "${CMAKE_SOURCE_DIR}/dependencies/xcmdline")

    # path to xerr for the include files
    set(XCMDLINE_INCLUDE_LIST "${ROOT}"
    )
    
    # Define file list
    set(XCMDLINE_FILES_LIST "${ROOT}/source/xcmdline_parser.h"
                            "${ROOT}/readme.md"
    )

    # Set global properties
    set_property(GLOBAL PROPERTY XCMDLINE_GROUP "dependencies/xcore/xcmdline")
    set_property(GLOBAL PROPERTY XCMDLINE_FILES "${XCMDLINE_FILES_LIST}")
    set_property(GLOBAL PROPERTY XCMDLINE_INCLUDES "${XCMDLINE_INCLUDE_LIST}")
    set_property(GLOBAL APPEND PROPERTY COMPONENT_REGISTRY "XCMDLINE")
endif()
