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

    if(MSVC)
        set(FILE_GROUP "cmake/gtest.cmake")
        source_group("Extras" FILES ${FILE_GROUP})
        list(APPEND SOURCE_FILES ${FILE_GROUP})
    endif()

    set(FILE_GROUP
        "test.h"
        "test_format.cpp"
        "test_misc.cpp"
        "test_vi_timing.cpp"
    )
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME})
#    set_target_properties(
#        gtest
#    PROPERTIES
#        FOLDER "ThirdParty/GTest"
#    )

    target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest)
endif()

if (VI_TM_ENABLE_BENCHMARK)
    include(cmake/gbenchmark.cmake)

    if(MSVC)
        set(FILE_GROUP
          "cmake/gbenchmark.cmake"
        )
        source_group("Extras" FILES ${FILE_GROUP})
        list(APPEND SOURCE_FILES ${FILE_GROUP})
    endif()
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

set(BUILD_SHARED_LIBS ${_original_BUILD_SHARED_LIBS})
