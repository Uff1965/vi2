# File: "vi2/examples/lua_ext/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(test_lua_ext LANGUAGES CXX)

add_library(${PROJECT_NAME} MODULE "vi_timing_lua.cpp" "vi_timing_lua.def" "vi_timing.lua")
set_source_files_properties(vi_timing.lua PROPERTIES HEADER_FILE_ONLY TRUE)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    lua_core
    vi_timing
)
target_include_directories(${PROJECT_NAME}
PRIVATE
    lua_core
)

set_target_properties(${PROJECT_NAME}
PROPERTIES
    FOLDER "Examples"
    OUTPUT_NAME "vi_timing_lua$<$<CONFIG:Debug>:_d>"
    OUTPUT_NAME_DEBUG "vi_timing_lua_d"
    OUTPUT_NAME_RELEASE "vi_timing_lua"
    POSITION_INDEPENDENT_CODE ON
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

add_test(
    NAME ${PROJECT_NAME}
    COMMAND  "./lua$<$<CONFIG:Debug>:_d>"
        "${CMAKE_CURRENT_SOURCE_DIR}/vi_timing.lua"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
set_tests_properties(${PROJECT_NAME}
PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "VI_TM_NO_PAUSE=1;CONFIG=$<CONFIG>"
)
