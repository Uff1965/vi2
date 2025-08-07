# File: "vi2/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

# Options:
option(VI_TIMING_ENABLE_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static ones" OFF)
option(VI_TM_STAT_USE_BASE "Using basic statistics collection (cnt, sum)." ON)
option(VI_TM_STAT_USE_FILTER "Using correlation coefficients and bounce filtering in measurements." ON)
option(VI_TM_STAT_USE_MINMAX "To store minimum and maximum measurement values." OFF)
option(VI_TM_THREADSAFE "Enable thread safety." ON)

include(FetchContent)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable folder structure in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/HEAD"
    "${CMAKE_SOURCE_DIR}/.git/packed-refs"
)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake") # Include the script to generate version information.

# Set project name and version based on the generated version information.
project(vi_timing VERSION ${GIT_VERSION_NUMBER})

add_subdirectory(src)

if (VI_TIMING_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#add_subdirectory(benchmarks)
#add_subdirectory(examples)
