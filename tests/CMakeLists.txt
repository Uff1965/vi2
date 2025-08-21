# File: "vi2/tests/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(vi_timing_tests LANGUAGES C CXX)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE vi_timing)

set_target_properties(${PROJECT_NAME}
PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}$<IF:$<OR:$<BOOL:${BUILD_SHARED_LIBS}>,$<CONFIG:Debug>>,_,>$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>$<IF:$<CONFIG:Debug>,d,>
    OUTPUT_NAME_DEBUG "${PROJECT_NAME}_$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>d"
    OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,_s,>"
)

set(_original_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF) # Disable shared libraries for tests to ensure they are built as static libraries.

set(SOURCE_FILES)

if (VI_TM_ENABLE_TESTS)
    include(cmake/gtest.cmake)

    set(FILE_GROUP "cmake/gtest.cmake")
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    set(FILE_GROUP
        "test.h"
        "test_format.cpp"
        "test_misc.cpp"
        "test_vi_timing.cpp"
    )
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTest::gtest
#       GTest::gtest_main
#       GMock::gmock
#       GMock::gmock_main
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        VI_HAS_GTEST=1
    )

    gtest_discover_tests(${PROJECT_NAME})
endif()

if (VI_TM_ENABLE_BENCHMARK)
    include(cmake/gbenchmark.cmake)

    set(FILE_GROUP "cmake/gbenchmark.cmake")
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    set(FILE_GROUP
        "benchmark.h"
        "benchmark.cpp"
    )
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        benchmark::benchmark
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        VI_HAS_GBENCHMARK=1
    )
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

set(BUILD_SHARED_LIBS ${_original_BUILD_SHARED_LIBS})
