# File: "vi2/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

# Options:
option(BUILD_SHARED_LIBS "Build shared libraries instead of static ones" OFF)
option(VI_TIMING_ENABLE_TESTS "Build unit tests" ON)

# Enable folder structure in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/HEAD"
    "${CMAKE_SOURCE_DIR}/.git/packed-refs"
)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/GenerateVersion.cmake") # Include the script to generate version information.

# Set project name and version based on the generated version information.
project(vi_timing VERSION ${GIT_VERSION_NUMBER})

add_subdirectory(src)

if (VI_TIMING_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

#add_subdirectory(benchmarks)
#add_subdirectory(examples)
