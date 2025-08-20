# File: "vi2/tests/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(vi_timing_examples LANGUAGES C CXX)

#add_executable(${PROJECT_NAME})
#
#target_link_libraries(${PROJECT_NAME} PRIVATE vi_timing)
#
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
