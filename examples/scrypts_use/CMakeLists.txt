# File: "vi2/examples/scrypts_use/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(scrypts_use LANGUAGES CXX)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE vi_timing)

set_target_properties(${PROJECT_NAME}
PROPERTIES
	FOLDER "Examples"
	OUTPUT_NAME "${PROJECT_NAME}$<$<CONFIG:Debug>:_d>"
	OUTPUT_NAME_DEBUG "${PROJECT_NAME}_d"
	OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
	POSITION_INDEPENDENT_CODE ON
)

set(SOURCES main.cpp header.h)

if(VI_TM_ENABLE_PYTHON)
	# Find Python3 and determine extension suffix!
	find_package(Python3 3.11 COMPONENTS Interpreter Development REQUIRED)
	message(STATUS "Python3 found version: ${Python3_VERSION}")

	target_link_libraries(${PROJECT_NAME} PRIVATE ${Python3_LIBRARIES})
	target_include_directories(${PROJECT_NAME} PRIVATE ${Python3_INCLUDE_DIRS})

	list(APPEND SOURCES python.cpp)
endif()

if(VI_TM_ENABLE_LUA)
	target_link_libraries(${PROJECT_NAME} PRIVATE lua_core)

	list(APPEND SOURCES lua.cpp)
endif()

if(VI_TM_ENABLE_QJS)
	include(${CMAKE_SOURCE_DIR}/cmake/QuickJS-NG.cmake)

	target_link_libraries(${PROJECT_NAME} PRIVATE qjs)

	list(APPEND SOURCES qjs.cpp)
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_compile_definitions(${PROJECT_NAME}
PRIVATE
	$<$<CONFIG:Release>: NDEBUG>
PUBLIC
)

if (WIN32)
	target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
		NOMINMAX # WinAPI
		$<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
	)

	target_compile_options(${PROJECT_NAME}
	PRIVATE
		/MP /nologo # Enable multi-processor compilation, disable logo.
		/W4 # Set warning level 4.
		/EHsc # Enable C++ exceptions.
		/arch:AVX2 # Enable use AVX2 instruction set.
		/Zi # Generate debug info
		/Zc:__cplusplus # By default, Visual Studio always returns the value 199711L for the __cplusplus preprocessor macro.
		$<$<CONFIG:Release>: /Ox /GL> # Ox include /O2 /Ob2 /Og /Oi /Ot /Oy /Gs /GF /Gy
		$<$<CONFIG:Debug>: /Od /RTC1 /Zi /Ob0> # Enable runtime checks, debug info, no optimizations, disable function inlining.
	)

	target_link_options(${PROJECT_NAME}
	PRIVATE
		$<$<CONFIG:Release>: /LTCG> # Link Time Code Generation for Release builds.
		$<$<CONFIG:Debug>: /DEBUG> # Generate debug info for Debug builds.
	)
elseif(UNIX)
	target_compile_options(${PROJECT_NAME}
	PRIVATE
		-fvisibility=hidden # Hide symbols by default, only export those marked with VI_TM_EXPORTS.
		-Wno-psabi # suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
		-march=native # Switch on all instruction sets supported by the host CPU. Support FMA, AVX, AVX2, etc.
		$<$<CONFIG:Release>: -O3 -flto=auto -s> # Optimize for speed, strip symbols.
	)

	target_link_options(${PROJECT_NAME}
	PRIVATE
		"$<$<CONFIG:Release>:-flto=auto>"
	)
endif()

#add_test(NAME ${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
#set_tests_properties(${PROJECT_NAME}
#PROPERTIES
##    DISABLED $<NOT:$<CONFIG:Release>>
##    TIMEOUT 450
#    ENVIRONMENT "VI_TM_NO_PAUSE=1"
##    PASS_REGULAR_EXPRESSION "2\\. global_(mod|lib):[^\\n]*$"
#)
