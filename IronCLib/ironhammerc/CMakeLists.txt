cmake_minimum_required(VERSION 3.20)
project(PortabilityTest C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(hammer_ironclib 
    main.c
)

target_include_directories(hammer_ironclib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../ironclib
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hammer_ironclib PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

if (MSVC)
    target_compile_options(hammer_ironclib PRIVATE
        /W4
        /WX
        /wd4127  # macro pattern (do { } while(0))
    )
endif()