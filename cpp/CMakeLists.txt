cmake_minimum_required(VERSION 3.20)
project(ShallowWaterSolver VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_GPU "Enable GPU acceleration with Kokkos" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Optimization flags for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3 -march=native)
    endif()
endif()

# Find dependencies
find_package(Threads REQUIRED)

# Kokkos support - handles CPU and GPU backends
find_package(Kokkos QUIET)
if(Kokkos_FOUND)
    add_compile_definitions(USE_KOKKOS)
else()
    message(STATUS "Kokkos not found - building CPU-only version")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/mesh.cpp
    src/solver.cpp
    src/io.cpp
    src/boundary.cpp
)

if(Kokkos_FOUND)
    list(APPEND SOURCES src/gpu_solver.cpp)
endif()

# Create library
add_library(swe_lib STATIC ${SOURCES})
target_link_libraries(swe_lib PUBLIC Threads::Threads)

if(Kokkos_FOUND)
    target_link_libraries(swe_lib PUBLIC Kokkos::kokkos)
endif()

# Main executable
add_executable(shallow-water-solver src/main.cpp)
target_link_libraries(shallow-water-solver PRIVATE swe_lib)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
install(TARGETS shallow-water-solver
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)
