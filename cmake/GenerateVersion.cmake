cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

find_package(Git QUIET)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --tags --long --always --abbrev --dirty --broken
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DESCRIBE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
		COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=iso
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DATETIME
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	string(REGEX MATCH "^(v\\.?)?(([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?(\\.([0-9]+))?)-.*" MATCHED "${GIT_DESCRIBE}")
	if(MATCHED)
		set(GIT_VERSION_NUMBER "${CMAKE_MATCH_2}")
	else()
		set(GIT_VERSION_NUMBER "0.0.0")
	endif()
else()
	set(GIT_DESCRIBE "")
	set(GIT_COMMIT "unknown")
	set(GIT_DATETIME "unknown")
	set(GIT_VERSION_NUMBER "0.0.0")
endif()
