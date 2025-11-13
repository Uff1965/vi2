# File: "vi2/examples/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(Examples LANGUAGES CXX)

add_executable(1ex_simple "ex_simple.cpp")
set_target_properties(1ex_simple PROPERTIES FOLDER "Examples")
target_link_libraries(1ex_simple PRIVATE vi_timing)
set_target_properties(1ex_simple
PROPERTIES
    OUTPUT_NAME "1ex_simple$<IF:$<OR:$<BOOL:${VI_TM_NAME_SUFFIX}>,$<CONFIG:Debug>>,_,>${VI_TM_NAME_SUFFIX}$<IF:$<CONFIG:Debug>,d,>${VI_TM_VER_SUFFIX}"
    OUTPUT_NAME_DEBUG "1ex_simple_${VI_TM_NAME_SUFFIX}d${VI_TM_VER_SUFFIX}"
    OUTPUT_NAME_RELEASE "1ex_simple$<IF:$<BOOL:${VI_TM_NAME_SUFFIX}>,_${VI_TM_NAME_SUFFIX},>${VI_TM_VER_SUFFIX}"
)

add_executable(2ex_simple_disabled "ex_simple.cpp")
set_target_properties(2ex_simple_disabled PROPERTIES FOLDER "Examples")
target_compile_definitions(2ex_simple_disabled PRIVATE VI_TM_DISABLE=1)
target_link_libraries(2ex_simple_disabled PRIVATE vi_timing)
set_target_properties(2ex_simple_disabled
PROPERTIES
    OUTPUT_NAME "2ex_simple_disabled<IF:$<OR:$<BOOL:${VI_TM_NAME_SUFFIX}>,$<CONFIG:Debug>>,_,>${VI_TM_NAME_SUFFIX}$<IF:$<CONFIG:Debug>,d,>${VI_TM_VER_SUFFIX}"
    OUTPUT_NAME_DEBUG "2ex_simple_disabled${VI_TM_NAME_SUFFIX}d${VI_TM_VER_SUFFIX}"
    OUTPUT_NAME_RELEASE "2ex_simple_disabled$<IF:$<BOOL:${VI_TM_NAME_SUFFIX}>,_${VI_TM_NAME_SUFFIX},>${VI_TM_VER_SUFFIX}"
)

add_test(NAME ex_simple COMMAND 1ex_simple)
set_tests_properties(ex_simple
PROPERTIES
    PASS_REGULAR_EXPRESSION "#. Name +:"
)
    
add_test(NAME ex_disabled COMMAND 2ex_simple_disabled)
