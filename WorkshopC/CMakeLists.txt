cmake_minimum_required(VERSION 3.20)

project(workshopc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =====================================================
# Compile definitions (safe + cross-platform)
# =====================================================
add_compile_definitions(
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _FILE_OFFSET_BITS=64
    __STDC_CONSTANT_MACROS
    __STDC_FORMAT_MACROS
    __STDC_LIMIT_MACROS
)

# =====================================================
# LLVM / Clang
# =====================================================
set(LLVM_USE_STATIC_LIBS ON)  # or OFF for DLLs
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR: ${LLVM_DIR}")
message(STATUS "Clang_DIR: ${Clang_DIR}")

# Include LLVM/Clang headers
include_directories(SYSTEM
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# LLVM compile definitions (important for MSYS2 + Windows)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_compile_definitions(${LLVM_DEFINITIONS_LIST})

# =====================================================
# Source files
# =====================================================
set(SOURCES
    src/main.cpp
    src/config_parser.cpp
    src/diagnostics.cpp
    src/suppression_manager.cpp
    src/struct_database.cpp
    src/finder_and_finalizer_consumer.cpp
    src/rule_suppression_reason.cpp
    src/rule_suppression_balance.cpp
    src/rule_enum.cpp
    src/rule_private.cpp
    src/rule_private_alternative.cpp
    src/rule_function_pointer.cpp
    src/rule_typedef_struct.cpp
    src/rule_assignment.cpp
    src/rule_prefix_namespace.cpp
    src/rule_null_check.cpp
    src/rule_arg_ptr_move.cpp
    src/rule_arg_ptr_move_callsite.cpp
    src/rule_arg_ptr_use_after_move.cpp
    src/struct_database_rule.cpp
    src/struct_init_rule.cpp
    src/struct_cleanup_rule.cpp
    src/struct_raii_discard_rule.cpp
    src/struct_destroy_definition_rule.cpp
    src/rule_restricted_malloc.cpp
    src/rule_single_return.cpp
    src/rule_strict_switch.cpp
    src/rule_global_variable.cpp
    src/rule_reference_pointer.cpp
    src/rule_function_pointer_tags.cpp
    src/rule_function_discard.cpp
    src/rule_array_struct.cpp
    src/rule_const_field.cpp
    src/rule_no_goto.cpp
    src/rule_vtable.cpp
    src/rule_interface.cpp
    src/rule_span_struct.cpp
)

add_executable(workshopc ${SOURCES})

# Generate C++ headers containing the built-in YAML configs. Track each
# generated header so config or script edits rerun the converter during builds.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

file(GLOB_RECURSE WORKSHOPC_CONFIG_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/default/configs/*.workshopc.yaml"
)

set(WORKSHOPC_GENERATED_CONFIG_HEADERS)
foreach(CONFIG_FILE IN LISTS WORKSHOPC_CONFIG_FILES)
    file(RELATIVE_PATH CONFIG_RELATIVE_PATH
        "${CMAKE_SOURCE_DIR}/default/configs"
        "${CONFIG_FILE}"
    )
    string(REGEX REPLACE "\\.workshopc\\.yaml$" ".config.hpp"
        CONFIG_HEADER_RELATIVE_PATH
        "${CONFIG_RELATIVE_PATH}"
    )
    list(APPEND WORKSHOPC_GENERATED_CONFIG_HEADERS
        "${CMAKE_SOURCE_DIR}/generated/${CONFIG_HEADER_RELATIVE_PATH}"
    )
endforeach()

add_custom_command(
    OUTPUT ${WORKSHOPC_GENERATED_CONFIG_HEADERS}
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/scripts/config_to_string.py"
        "${CMAKE_SOURCE_DIR}/default/configs"
        "${CMAKE_SOURCE_DIR}/generated"
    DEPENDS
        ${WORKSHOPC_CONFIG_FILES}
        "${CMAKE_SOURCE_DIR}/scripts/config_to_string.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating embedded config headers"
    VERBATIM
)

add_custom_target(generate_workshopc_configs
    DEPENDS ${WORKSHOPC_GENERATED_CONFIG_HEADERS}
)
add_dependencies(workshopc generate_workshopc_configs)
target_include_directories(workshopc PRIVATE
    "${CMAKE_SOURCE_DIR}/generated"
)

# Clang's own builtin headers (stddef.h, mm_malloc.h, ...) live in its
# resource directory. The parser looks for it next to its executable by
# default, which fails when it is not installed next to clang, so the
# location of the clang it was built against is passed in as a fallback.
set(WORKSHOPC_CLANG_RESOURCE_DIR "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION_MAJOR}")
message(STATUS "Clang resource dir: ${WORKSHOPC_CLANG_RESOURCE_DIR}")

target_compile_definitions(workshopc PRIVATE
    "WORKSHOPC_CLANG_RESOURCE_DIR=\"${WORKSHOPC_CLANG_RESOURCE_DIR}\""
)

# =====================================================
# LLVM / Clang linking
# =====================================================
llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
)

target_link_libraries(workshopc PRIVATE
    # Clang
    clangTooling
    clangFrontend
    clangSerialization
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangASTMatchers
    clangAST
    clangLex
    clangBasic

    # LLVM
    ${LLVM_LIBS}
)

# =====================================================
# Platform-specific extras
# =====================================================
if (MINGW)
    target_link_libraries(workshopc PRIVATE
        ws2_32
        version
        bcrypt
    )
endif()

# =====================================================
# Release folder
# =====================================================
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release)

add_custom_command(TARGET workshopc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_DIR}
)

# Copy executable
add_custom_command(TARGET workshopc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:workshopc>
        ${RELEASE_DIR}/$<TARGET_FILE_NAME:workshopc>
)

# Copy defaults and licenses on every build, not only when the executable
# is linked again, so that a changed config or tag header always ends up
# in the release folder
add_custom_target(release_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/default/configs
        ${RELEASE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/default/tags
        ${RELEASE_DIR}/tags
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/../LICENSE
        ${RELEASE_DIR}/LICENSE
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/../THIRD-PARTY-NOTICES.txt
        ${RELEASE_DIR}/THIRD-PARTY-NOTICES.txt
    COMMENT "Copying the default config, tag headers and licenses to ${RELEASE_DIR}"
)