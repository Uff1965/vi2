FetchContent_Declare(
	googlebenchmark
	GIT_REPOSITORY https://github.com/google/benchmark.git
	GIT_TAG        v1.9.4
	GIT_PROGRESS   TRUE
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")

FetchContent_MakeAvailable(googlebenchmark)

set_target_properties(benchmark_main
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(benchmark
PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/debug
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/release
	DEBUG_POSTFIX "_d"
	FOLDER "3rdparty"
)
