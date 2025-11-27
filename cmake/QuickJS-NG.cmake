FetchContent_Declare(
  quickjs
  GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
  GIT_TAG        v0.11.0
)

# Build standard library modules as part of the library.
set(QJS_BUILD_LIBC ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(quickjs)

set_target_properties(qjs
PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/debug
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/release
	DEBUG_POSTFIX "_d"
	FOLDER "3rdparty"
)

set_target_properties(qjs_exe
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(api-test
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(function_source
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(qjsc
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(run-test262
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)

set_target_properties(unicode_gen
PROPERTIES
	EXCLUDE_FROM_ALL TRUE
	EXCLUDE_FROM_DEFAULT_BUILD TRUE
	FOLDER "3rdparty"
)
