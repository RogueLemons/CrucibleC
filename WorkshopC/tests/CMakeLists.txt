# Standalone project for the WorkshopC test files.
#
# This project is only ever CONFIGURED, never built. Configuring it
# writes compile_commands.json, which run_tests.py passes to the
# parser so that the test files are analyzed with their own compile
# flags instead of the flags of the parser's own C++ sources.
#
# Do not add_subdirectory() this from the main CMakeLists.txt.

cmake_minimum_required(VERSION 3.20)
project(WorkshopCTests LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Every test case is a folder cases/<name>/ holding <name>.c
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/cases/*/*.c
)

# The folder passed whole to the parser by the folder input test
file(GLOB_RECURSE FOLDER_INPUT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/folder_input/*.c
)

list(APPEND TEST_SOURCES ${FOLDER_INPUT_SOURCES})

add_library(workshopc_tests OBJECT ${TEST_SOURCES})

# Lets the tests include the shared "headers/...", "external/..." and
# "system/..." (headers marked with '#pragma GCC system_header')
target_include_directories(workshopc_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(workshopc_tests PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
