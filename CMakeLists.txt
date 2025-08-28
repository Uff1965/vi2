# File: "vi2/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)

# Options:
option(BUILD_SHARED_LIBS "Build shared libraries instead of static ones" OFF)
option(VI_TM_THREADSAFE "Enable thread safety." ON)

option(VI_TM_STAT_USE_RAW "Using RAW statistics collection (cnt, sum)." ON)
option(VI_TM_STAT_USE_FILTER "Using correlation coefficients and bounce filtering in measurements." ON)
option(VI_TM_STAT_USE_MINMAX "To store minimum and maximum measurement values." OFF)

option(VI_TM_ENABLE_TESTS "Build unit tests" ON)
option(VI_TM_ENABLE_BENCHMARK "Build benchmark" ON)
option(VI_TM_ENABLE_EXAMPLES "Build examples" ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configs" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(BUILD_SHARED_LIBS)
    set(VI_TM_SHARED 1)
endif()

include("cmake/GenerateVersion.cmake") # Include the script to generate version information.
vi_get_version_from_git(VI_GIT)
message(STATUS "VI_GIT_DESCRIBE: ${VI_GIT_DESCRIBE}")
message(STATUS "\tVI_GIT_VERSION_NUMBER: ${VI_GIT_VERSION_NUMBER}")
message(STATUS "\tVI_GIT_VERSION_LABEL: ${VI_GIT_VERSION_LABEL}")

include("cmake/make_suffix.cmake")
vi_make_name_suffix(VI_TM_NAME_SUFFIX)
message(STATUS "VI_TM_NAME_SUFFIX: ${VI_TM_NAME_SUFFIX}")

set(VI_TM_OUTPUT "${CMAKE_SOURCE_DIR}/bin")
foreach(CONFIG_TYPE IN ITEMS "" "_DEBUG" "_RELEASE")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY${CONFIG_TYPE}  ${VI_TM_OUTPUT})
endforeach()

include(FetchContent)
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Enable folder structure in IDEs

# Set project name and version based on the generated version information.
project(vi VERSION ${VI_GIT_VERSION_NUMBER} LANGUAGES NONE)

if (VI_TM_ENABLE_TESTS)
    include(CTest)
endif()

add_subdirectory(src)

if (VI_TM_ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

if ( BUILD_TESTING OR VI_TM_ENABLE_BENCHMARK)
    add_subdirectory(tests)
endif()
