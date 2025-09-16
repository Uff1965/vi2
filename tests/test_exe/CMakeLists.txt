# File: "vi2/tests/exe/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)
project(test_exe LANGUAGES C CXX)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")

target_link_libraries(${PROJECT_NAME}
PRIVATE
    test_lib
    vi_timing
)

set_target_properties(${PROJECT_NAME}
PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}$<IF:$<OR:$<BOOL:${VI_TM_NAME_SUFFIX}>,$<CONFIG:Debug>>,_,>${VI_TM_NAME_SUFFIX}$<IF:$<CONFIG:Debug>,d,>
    OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${VI_TM_NAME_SUFFIX}d"
    OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<IF:$<BOOL:${VI_TM_NAME_SUFFIX}>,_${VI_TM_NAME_SUFFIX},>"
)

set(_original_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF) # Disable shared libraries for tests to ensure they are built as static libraries.

set(SOURCE_FILES)
list(APPEND SOURCE_FILES "test_vi_timing.cpp")

if (VI_TM_ENABLE_TESTS)
    set(FILE_GROUP)
    list(APPEND FILE_GROUP "../cmake/gtest.cmake")
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    set(FILE_GROUP
        "test.h"
        "test_format.cpp"
        "test_misc.cpp"
        "test_timing.cpp"
    )

    if(VI_TM_THREADSAFE)
        list(APPEND FILE_GROUP "test_multithreaded.cpp")
    endif()

    list(APPEND SOURCE_FILES ${FILE_GROUP})

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTest::gtest
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        VI_HAS_GTEST=1
    )

    gtest_discover_tests(${PROJECT_NAME}
        DISCOVERY_TIMEOUT 30
    )

    add_test(NAME ex_simple COMMAND 1ex_simple)
    set_tests_properties(ex_simple
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^Hello, World!"
        PASS_REGULAR_EXPRESSION "#  Name \\(v\\)\\.+: *Avg\\. \\+\\/\\- +CV *~= +Total *\\/ +Cnt\\."
    )
    
    add_test(NAME ex_disabled COMMAND 2ex_simple_disabled)
    set_tests_properties(ex_disabled
    PROPERTIES
        PASS_REGULAR_EXPRESSION "^Hello, World!"
    )
endif()

if (VI_TM_ENABLE_BENCHMARK)
    set(FILE_GROUP)
    list(APPEND FILE_GROUP "../cmake/gbenchmark.cmake")
    source_group("Extras" FILES ${FILE_GROUP})
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    set(FILE_GROUP
        "benchmark.h"
        "benchmark.cpp"
    )
    list(APPEND SOURCE_FILES ${FILE_GROUP})

    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        benchmark::benchmark
    )

    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        VI_HAS_GBENCHMARK=1
    )
endif()

set(FILE_GROUP "tests.pvsconfig")
source_group("Extras" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

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
        $<$<CONFIG:Release>: -O3 -flto -s> # Optimize for speed, strip symbols.
    )

    target_link_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:-flto>")
endif()

set(BUILD_SHARED_LIBS ${_original_BUILD_SHARED_LIBS})
