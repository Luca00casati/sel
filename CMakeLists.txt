cmake_minimum_required(VERSION 3.16)

project(sel
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sel
    src/main.cpp
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
