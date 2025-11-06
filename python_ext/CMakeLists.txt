# File: "vi2/python_ext/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(test_python_ext LANGUAGES CXX)

# Find Python3 and determine extension suffix!
message("")
find_package(Python3 3.14 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 found version: ${Python3_VERSION}")
if(NOT Python3_EXTENSION_SUFFIX OR Python3_EXTENSION_SUFFIX STREQUAL "")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import importlib.machinery as m; s = m.EXTENSION_SUFFIXES[0] if m.EXTENSION_SUFFIXES else ''; print(s)"
        OUTPUT_VARIABLE PY_EXT_SUFFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(PY_EXT_SUFFIX)
        set(Python3_EXTENSION_SUFFIX "${PY_EXT_SUFFIX}")
    else()
        if(WIN32)
            set(Python3_EXTENSION_SUFFIX ".pyd")
        elseif(UNIX)
            set(Python3_EXTENSION_SUFFIX ".so")
        else()
            set(Python3_EXTENSION_SUFFIX ".so")
        endif()
    endif()
endif()
message(STATUS "\tPython3_EXTENSION_SUFFIX: ${Python3_EXTENSION_SUFFIX}\n")

add_library(${PROJECT_NAME} MODULE "vi_timing_py.cpp" "vi_timing.py")
set_source_files_properties(vi_timing.py PROPERTIES HEADER_FILE_ONLY TRUE)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    ${Python3_LIBRARIES}
    vi_timing
)
target_include_directories(${PROJECT_NAME}
PRIVATE
    ${Python3_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME}
PROPERTIES
    PREFIX ""
    SUFFIX "${Python3_EXTENSION_SUFFIX}"
    FOLDER "Examples"
    OUTPUT_NAME "vi_timing_py$<$<CONFIG:Debug>:_d>"
    OUTPUT_NAME_DEBUG "vi_timing_py_d"
    OUTPUT_NAME_RELEASE "vi_timing_py"
    POSITION_INDEPENDENT_CODE ON
)

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
