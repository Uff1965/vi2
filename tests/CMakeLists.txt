# File: "vi2/tests/CMakeLists.txt"

set(BUILD_GMOCK OFF CACHE BOOL "Disable building GoogleMock")
set(BUILD_GTEST ON CACHE BOOL "Enable building GoogleTest")

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


set(SOURCE_FILES
    "test.h"
    "test_format.cpp"
    "test_misc.cpp"
    "test_vi_timing.cpp"
)
#source_group("Source files" FILES ${SOURCE_FILES})

add_executable(vi_timing_tests ${SOURCE_FILES})

include(GoogleTest)
gtest_discover_tests(vi_timing_tests)
set_target_properties(gtest PROPERTIES FOLDER "ThirdParty/GTest")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty/GTest")

target_link_libraries( vi_timing_tests
PRIVATE
  vi_timing
#  gtest_main
  gtest
)
