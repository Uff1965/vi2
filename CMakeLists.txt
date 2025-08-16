# File: "vi2/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)

# Options:
option(VI_TIMING_ENABLE_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static ones" OFF)
option(VI_TM_STAT_USE_BASE "Using basic statistics collection (cnt, sum)." ON)
option(VI_TM_STAT_USE_FILTER "Using correlation coefficients and bounce filtering in measurements." ON)
option(VI_TM_STAT_USE_MINMAX "To store minimum and maximum measurement values." OFF)
option(VI_TM_THREADSAFE "Enable thread safety." ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configs" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

# Enable folder structure in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/HEAD"
    "${CMAKE_SOURCE_DIR}/.git/packed-refs"
)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake") # Include the script to generate version information.

set(VI_TM_OUTPUT "${CMAKE_SOURCE_DIR}/bin")
foreach(CONFIG_TYPE IN ITEMS "" "_DEBUG" "_RELEASE")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
endforeach()

# Set project name and version based on the generated version information.
project(vi VERSION ${GIT_VERSION_NUMBER} LANGUAGES NONE)

add_subdirectory(src)

if (VI_TIMING_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#add_subdirectory(benchmarks)
#add_subdirectory(examples)
