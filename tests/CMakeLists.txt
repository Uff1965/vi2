# File: "vi2/tests/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(vi_timing_tests LANGUAGES C CXX)

set(SOURCE_FILES
    "test.h"
    "test_format.cpp"
    "test_misc.cpp"
    "test_vi_timing.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}$<IF:$<OR:$<BOOL:${BUILD_SHARED_LIBS}>,$<CONFIG:Debug>>,_,>$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>$<IF:$<CONFIG:Debug>,d,>
  OUTPUT_NAME_DEBUG "${PROJECT_NAME}_$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>d"
  OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,_s,>"
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(gtest PROPERTIES FOLDER "ThirdParty/GTest")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty/GTest")

target_link_libraries( ${PROJECT_NAME}
PRIVATE
  vi_timing
#  gtest_main
  gtest
)
