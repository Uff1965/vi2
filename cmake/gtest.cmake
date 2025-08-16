set(_original_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

set(BUILD_GMOCK OFF CACHE BOOL "Disable building GoogleMock")
set(BUILD_GTEST ON CACHE BOOL "Enable building GoogleTest")

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_target_properties(gtest PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/debug
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/release
)

set(BUILD_SHARED_LIBS ${_original_BUILD_SHARED_LIBS})
