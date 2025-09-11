cmake_minimum_required(VERSION 3.22)

project(vi_timing_tests LANGUAGES NONE)

if (VI_TM_ENABLE_TESTS)
    include(cmake/gtest.cmake)
endif()

if (VI_TM_ENABLE_BENCHMARK)
    include(cmake/gbenchmark.cmake)
endif()

add_subdirectory(test_lib)
add_subdirectory(test_exe)
