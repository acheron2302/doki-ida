cmake_minimum_required(VERSION 3.27)
project(doki_theme CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# IDA SDK via ida-cmake (https://github.com/allthingsida/ida-cmake), which is
# embedded in the official HexRaysSA/ida-sdk under $IDASDK/ida-cmake.
# Requires the IDASDK environment variable to point at the SDK root.
# ---------------------------------------------------------------------------
if(NOT DEFINED ENV{IDASDK})
    message(FATAL_ERROR "IDASDK environment variable is not set. Point it at the IDA SDK root.")
endif()

include($ENV{IDASDK}/ida-cmake/bootstrap.cmake)
find_package(idasdk REQUIRED)

set(DOKI_VERSION "0.1.0")

# ---------------------------------------------------------------------------
# Sources. Phase 1 is just the skeleton; later phases append to DOKI_SOURCES.
# ---------------------------------------------------------------------------
set(DOKI_SOURCES
    src/plugin.cpp
    src/doki/theme.cpp
    src/doki/registry.cpp
    src/doki/paths.cpp
    src/doki/color.cpp
    src/doki/palette.cpp
    src/doki/css.cpp
    src/doki/installer.cpp
    src/doki/apply.cpp
    src/doki/ui.cpp
    src/doki/config.cpp
    src/doki/selftest.cpp
)

ida_add_plugin(doki_theme
    SOURCES
        ${DOKI_SOURCES}
)

target_include_directories(doki_theme PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_definitions(doki_theme PRIVATE
    DOKI_VERSION="${DOKI_VERSION}"
)

# nlohmann/json is a heavy single header: MSVC needs /bigobj and UTF-8 source.
if(MSVC)
    target_compile_options(doki_theme PRIVATE /bigobj /utf-8)
endif()

# Packaging: stage DLL + definitions + assets into a distributable zip.
if(WIN32)
    add_custom_target(package
        COMMAND powershell -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/package.ps1"
                -Version "${DOKI_VERSION}"
        DEPENDS doki_theme
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Packaging doki-theme-ida v${DOKI_VERSION}")
endif()
