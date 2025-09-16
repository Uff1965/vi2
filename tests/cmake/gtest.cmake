FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.17.0
#	GIT_PROGRESS   TRUE
)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
#FetchContent_Populate(googletest)
#add_subdirectory(${googletest_SOURCE_DIR}/googletest ${googletest_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

set_target_properties(gtest_main
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(gtest
PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/debug
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/release
	DEBUG_POSTFIX "_d"
	FOLDER "3rdparty"
)

include(GoogleTest)
