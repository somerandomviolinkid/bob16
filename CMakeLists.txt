cmake_minimum_required(VERSION 3.20)
project(bob16 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING FORCE "Choose build type: Debug Release")
    message(STATUS "No build type specified, defaulting to Debug")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(bob16 src/main.c src/cpu.c src/assembler.c)

target_include_directories(bob16 PRIVATE include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug build (Sanitizers enabled, -O0, -g)")
    target_compile_options(bob16 PRIVATE -fsanitize=address -fsanitize=undefined -g -O0)
    target_link_options(bob16 PRIVATE -fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring Release build (-O3, -DNDEBUG)")
    target_compile_options(bob16 PRIVATE -DNDEBUG -O3)
endif()