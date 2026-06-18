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
    src/doki/overlay.cpp
    src/doki/assets.cpp
)

# Locate Qt6 for the plugin target.
# Priority for Qt6_DIR (pointing at lib/cmake/Qt6):
#   1. Cached Qt6_DIR (set via -DQt6_DIR=... on the command line).
#   2. DOKI_QT_DIR environment variable -> $DOKI_QT_DIR/lib/cmake/Qt6
#      (use this to point at a local Qt, e.g. E:/dev/qt-yusa/x64win).
#   3. CMAKE_PREFIX_PATH inherited from -DCMAKE_PREFIX_PATH=... (CI uses this).
# We intentionally do not hardcode any developer-specific Qt path here.
if(NOT DEFINED Qt6_DIR AND DEFINED ENV{DOKI_QT_DIR})
    set(Qt6_DIR "$ENV{DOKI_QT_DIR}/lib/cmake/Qt6" CACHE PATH "Qt6 CMake directory")
    message(STATUS "Using DOKI_QT_DIR for Qt6: $ENV{DOKI_QT_DIR}")
endif()

# Explicit early find_package so we can override imported target properties
# (IDA Qt import libs) before ida_add_plugin appends Qt6::* to the link list.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Override Qt6::{Core,Gui,Widgets} import libs with the IDA SDK's QT_NAMESPACE=QT
# Qt import libraries (idasdk/src/lib/x64_win_qt). Linking against a public Qt
# import lib would pull in non-namespaced Qt symbols and break IDA at runtime.
set(IDA_QT_LIB_DIR "" CACHE PATH "IDA SDK QT_NAMESPACE=QT Qt import library directory")
if(WIN32)
    if(NOT IDA_QT_LIB_DIR AND DEFINED ENV{IDA_QT_LIB_DIR})
        set(IDA_QT_LIB_DIR "$ENV{IDA_QT_LIB_DIR}")
    endif()
    if(NOT IDA_QT_LIB_DIR AND DEFINED ENV{IDASDK})
        if(EXISTS "$ENV{IDASDK}/src/lib/x64_win_qt")
            set(IDA_QT_LIB_DIR "$ENV{IDASDK}/src/lib/x64_win_qt")
        elseif(EXISTS "$ENV{IDASDK}/lib/x64_win_qt")
            set(IDA_QT_LIB_DIR "$ENV{IDASDK}/lib/x64_win_qt")
        endif()
    endif()

    if(IDA_QT_LIB_DIR)
        message(STATUS "Overriding Qt6 import libs with IDA SDK Qt libs from: ${IDA_QT_LIB_DIR}")
        foreach(module Core Gui Widgets)
            set(lib_path "${IDA_QT_LIB_DIR}/Qt6${module}.lib")
            if(NOT EXISTS "${lib_path}")
                message(FATAL_ERROR "Missing IDA Qt import lib: ${lib_path}")
            endif()
            set_property(TARGET Qt6::${module} PROPERTY IMPORTED_IMPLIB "${lib_path}")
            set_property(TARGET Qt6::${module} PROPERTY IMPORTED_IMPLIB_RELEASE "${lib_path}")
            set_property(TARGET Qt6::${module} PROPERTY IMPORTED_IMPLIB_RELWITHDEBINFO "${lib_path}")
            set_property(TARGET Qt6::${module} PROPERTY IMPORTED_IMPLIB_MINSIZEREL "${lib_path}")
            set_property(TARGET Qt6::${module} PROPERTY IMPORTED_IMPLIB_DEBUG "${lib_path}")
        endforeach()
    endif()
endif()

ida_add_plugin(doki_theme
    TYPE QT
    QT_COMPONENTS Core Gui Widgets Network
    SOURCES
        ${DOKI_SOURCES}
    DEFINES
        QT_NAMESPACE=QT
        QT_CORE_LIB
        QT_DLL
        QT_GUI_LIB
        QT_NETWORK_LIB
        QT_THREAD_SUPPORT
        QT_WIDGETS_LIB
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
