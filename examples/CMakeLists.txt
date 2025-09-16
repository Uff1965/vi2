# File: "vi2/examples/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(Examples LANGUAGES CXX)

add_executable(1ex_simple "ex_simple.cpp")
set_target_properties(1ex_simple PROPERTIES FOLDER "Examples")
target_link_libraries(1ex_simple PRIVATE vi_timing)

add_executable(2ex_simple_disabled "ex_simple.cpp")
set_target_properties(2ex_simple_disabled PROPERTIES FOLDER "Examples")
target_compile_definitions(2ex_simple_disabled PRIVATE VI_TM_DISABLE=1)
target_link_libraries(2ex_simple_disabled PRIVATE vi_timing)

#target_link_libraries(${PROJECT_NAME} PRIVATE vi_timing)

#set_target_properties(${PROJECT_NAME}
#PROPERTIES
#    OUTPUT_NAME ${PROJECT_NAME}$<IF:$<OR:$<BOOL:${BUILD_SHARED_LIBS}>,$<CONFIG:Debug>>,_,>$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>$<IF:$<CONFIG:Debug>,d,>
#    OUTPUT_NAME_DEBUG "${PROJECT_NAME}_$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,s,>d"
#    OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<IF:$<BOOL:${BUILD_SHARED_LIBS}>,_s,>"
#)
#
#set(SOURCE_FILES)
#
##...
#
#target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})
