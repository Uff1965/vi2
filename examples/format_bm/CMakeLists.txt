# File: "vi2/examples/format_bm/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(format_bm LANGUAGES CXX)

add_executable(${PROJECT_NAME}
	main.cpp
)

set_target_properties(${PROJECT_NAME}
PROPERTIES
	FOLDER "Examples"
	OUTPUT_NAME "${PROJECT_NAME}$<IF:$<OR:$<BOOL:${VI_TM_NAME_SUFFIX}>,$<CONFIG:Debug>>,_,>${VI_TM_NAME_SUFFIX}$<IF:$<CONFIG:Debug>,d,>${VI_TM_VER_SUFFIX}"
	OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${VI_TM_NAME_SUFFIX}d${VI_TM_VER_SUFFIX}"
	OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<IF:$<BOOL:${VI_TM_NAME_SUFFIX}>,_${VI_TM_NAME_SUFFIX},>${VI_TM_VER_SUFFIX}"
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

if (VI_TM_ENABLE_BENCHMARK)
	target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		VI_TM_ENABLE_BENCHMARK=1
	)

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        benchmark::benchmark
    )
endif()

target_link_libraries(${PROJECT_NAME}
PRIVATE
	vi_timing
)

add_test(
    NAME ${PROJECT_NAME}
    COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
)
