# File: "vi/src/CMakeLists.txt"
cmake_minimum_required(VERSION 3.22)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.out" @ONLY)

set(SOURCE_FILES) # Initialize the list of source files.

if(MSVC)
    set(FILE_GROUP
      "${CMAKE_SOURCE_DIR}/LICENSE"
      "${CMAKE_SOURCE_DIR}/README"
    )
    source_group("Extras" FILES ${FILE_GROUP})
endif()

set(FILE_GROUP
  "${CMAKE_SOURCE_DIR}/include/vi_timing/vi_timing_aux.h"
  "${CMAKE_SOURCE_DIR}/include/vi_timing/vi_timing_proxy.hpp"
  "${CMAKE_SOURCE_DIR}/include/vi_timing/vi_timing.hpp"
  "${CMAKE_SOURCE_DIR}/include/vi_timing/vi_timing.h"
)
source_group("Interface files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/misc.h"
)
source_group("Header files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(FILE_GROUP
  "${CMAKE_CURRENT_SOURCE_DIR}/version.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/clock.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/misc.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/props.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/report.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/timing.cpp"
)
source_group("Source files" FILES ${FILE_GROUP})
list(APPEND SOURCE_FILES ${FILE_GROUP})

set(OUTPUT_NAME_SUFFIX "")

if(BUILD_SHARED_LIBS)
    string(APPEND OUTPUT_NAME_SUFFIX "s")
    set(TYPE_LIBRARY "SHARED")
endif()

add_library(vi_timing ${SOURCE_FILES})

set(VI_TM_OUTPUT_NAME_DEBUG "${PROJECT_NAME}_${OUTPUT_NAME_SUFFIX}d")
set(VI_TM_OUTPUT_NAME_RELEASE "${PROJECT_NAME}$<$<BOOL:${OUTPUT_NAME_SUFFIX}>:_>${OUTPUT_NAME_SUFFIX}")

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  C_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF

  DEFINE_SYMBOL "VI_TM_EXPORTS"

  OUTPUT_NAME_RELEASE "${VI_TM_OUTPUT_NAME_RELEASE}"
  OUTPUT_NAME_DEBUG "${VI_TM_OUTPUT_NAME_DEBUG}"

  #LIBRARY_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  #LIBRARY_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  #ARCHIVE_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  #ARCHIVE_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"
  #RUNTIME_OUTPUT_DIRECTORY_RELEASE	"${VI_OUT_DIR}"
  #RUNTIME_OUTPUT_DIRECTORY_DEBUG	"${VI_OUT_DIR}"

  VERSION    ${PROJECT_VERSION}
  SOVERSION  ${PROJECT_VERSION_MAJOR}

  POSITION_INDEPENDENT_CODE ON # Position Independent Code (-fPIC) for shared libraries on UNIX.
)

### Compiler ##################################################################
target_include_directories(vi_timing
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(NOT DEFINED VI_TM_DEBUG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VI_TM_DEBUG 1)
endif()

if(VI_TM_DEBUG)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_DEBUG=1
    )
endif()

target_compile_definitions(vi_timing
PRIVATE
  $<$<CONFIG:Release>: NDEBUG>
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_SHARED=1
    )
endif()

if (VI_TM_STAT_USE_BASE)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_STAT_USE_BASE=1
    )
endif()

if (VI_TM_STAT_USE_FILTER)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_STAT_USE_FILTER=1
    )
endif()

if (VI_TM_THREADSAFE)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_THREADSAFE=1
    )
endif()

if (VI_TM_STAT_USE_MINMAX)
    target_compile_definitions(vi_timing
    PUBLIC
        VI_TM_STAT_USE_MINMAX=1
    )
endif()

#target_precompile_headers(vi_timing
#PRIVATE
#    "CMAKE_CURRENT_SOURCE_DIR}/pch.hpp"
#)

if (WIN32)
    set_source_files_properties("CMAKE_CURRENT_SOURCE_DIR}/timing.cpp" PROPERTIES
        COMPILE_FLAGS "/fp:fast" # Enable fast floating-point model for 'timing.cpp'
        SKIP_PRECOMPILE_HEADERS ON
    )

    target_compile_definitions(vi_timing
    PRIVATE
      WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers.
      NOMINMAX # WinAPI
      WIN32 # Define WIN32 for Windows builds.
      _WINDOWS # Define _WINDOWS for Windows builds.
      $<$<CONFIG:Debug>: _DEBUG> # Microsoft debug RTL
    )

    target_compile_options(vi_timing
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

    if(BUILD_SHARED_LIBS)
        target_compile_options(vi_timing
        PRIVATE
            $<$<CONFIG:Release>: /MT>
            $<$<CONFIG:Debug>:   /MTd>
        )
    else()
        target_compile_options(vi_timing
        PRIVATE
            $<$<CONFIG:Release>: /MD>
            $<$<CONFIG:Debug>:   /MDd>
        )
    endif()
elseif (UNIX)
  set_source_files_properties("CMAKE_CURRENT_SOURCE_DIR}/timing.cpp" PROPERTIES
    COMPILE_FLAGS "-ffast-math" # Allow floating-point optimizations that may violate strict IEEE or ISO rules for 'timing.cpp'.
    SKIP_PRECOMPILE_HEADERS ON
  )

  target_compile_options(vi_timing
  PRIVATE
    -fvisibility=hidden # Hide symbols by default, only export those marked with VI_TM_EXPORTS.
    -Wno-psabi # suppress "note: parameter passing for argument of type <...> changed in GCC 7.1" message.
    -march=native # Switch on all instruction sets supported by the host CPU. Support FMA, AVX, AVX2, etc.
    $<$<CONFIG:Release>: -O3 -s> # Optimize for speed, strip symbols.
  )
endif()

#install:
include(GNUInstallDirs)
install(TARGETS vi_timing EXPORT vi_timingTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#Export + configuration:
install(EXPORT vi_timingTargets
    FILE vi_timingTargets.cmake
    NAMESPACE vi_timing::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vi_timing
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/vi_timingConfig.cmake.in
    ${CMAKE_SOURCE_DIR}/vi_timingConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vi_timing
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vi_timingConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vi_timing
)
