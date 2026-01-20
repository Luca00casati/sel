cmake_minimum_required(VERSION 3.16)

project(sel
    VERSION 1.0
    LANGUAGES C
)

# Use a modern C standard for _Alignof
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(sel
    src/main.c
    src/arena.c
    src/mystring.c
)

target_include_directories(sel
    PRIVATE include
)

# Warnings (recommended)
if (MSVC)
    target_compile_options(sel PRIVATE /W4)
else()
    target_compile_options(sel PRIVATE -Wall -Wextra -Wpedantic)
endif()
