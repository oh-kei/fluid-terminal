cmake_minimum_required(VERSION 3.20)
project(fluid_motion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fluid_demo
    src/main.cpp
    src/FluidSimulation.cpp
)

target_include_directories(fluid_demo PRIVATE src)

if (MSVC)
    target_compile_options(fluid_demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(fluid_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
