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
    src/struct_database_rule.cpp
    src/struct_init_rule.cpp
    src/struct_cleanup_rule.cpp
    src/struct_raii_discard_rule.cpp
    src/rule_restricted_malloc.cpp
)

add_executable(workshopc ${SOURCES})

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

# Copy defaults
add_custom_command(TARGET workshopc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/default/workshopc.config.yaml
        ${RELEASE_DIR}/workshopc.config.yaml
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/default/move_tags.h
        ${RELEASE_DIR}/move_tags.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/default/private_tag.h
        ${RELEASE_DIR}/private_tag.h
)

# Copy licenses
add_custom_command(TARGET workshopc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/../LICENSE
        ${RELEASE_DIR}/LICENSE
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/../THIRD-PARTY-NOTICES.txt
        ${RELEASE_DIR}/THIRD-PARTY-NOTICES.txt
)