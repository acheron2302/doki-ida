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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

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

# ---------------------------------------------------------------------------
# libcurl (statically linked into the plugin).
#
# The runtime CDN fetch uses libcurl, loaded statically through CMake
# FetchContent. This decouples HTTPS downloads from the Qt Network stack
# and from the Qt SSL/OpenSSL runtime libraries.
#
# - DOKI_USE_EMBEDDED_CURL=ON (default): Fetch libcurl 8.7.1, build it
#     statically, link it into doki_theme.dll. The plugin no longer needs
#     a separate libcurl deployment.
# - DOKI_USE_EMBEDDED_CURL=OFF: Skip FetchContent. The runtime falls
#     back to dynamic libcurl loading via QLibrary, so the user can
#     still ship a libcurl runtime DLL if they prefer.
# ---------------------------------------------------------------------------
option(DOKI_USE_EMBEDDED_CURL "Statically link libcurl into the plugin" ON)
set(DOKI_HAS_EMBEDDED_CURL 0)
if(DOKI_USE_EMBEDDED_CURL)
    include(FetchContent)
    set(FETCHCONTENT_QUIET FALSE)

    set(DOKI_LIBCURL_VERSION "8.7.1")
    set(DOKI_LIBCURL_URL
        "https://github.com/curl/curl/releases/download/curl-${DOKI_LIBCURL_VERSION//./_}/curl-${DOKI_LIBCURL_VERSION}.tar.xz"
        "https://curl.se/download/curl-${DOKI_LIBCURL_VERSION}.tar.xz")

    FetchContent_Declare(
        doki_libcurl
        URL         ${DOKI_LIBCURL_URL}
        URL_HASH    SHA256=6fea2aac6a4610fbd0400afb0bcddbe7258a64c63f1f68e5855ebc0c659710cd
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

    # Configure the libcurl build before fetching so that the subproject
    # honors our static + Schannel settings without prompting.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBS ON  CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE    OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING     OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(CURL_WERROR         OFF CACHE BOOL "" FORCE)
    set(PICKY_COMPILER      OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBIDN2    OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL     OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH     OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2    OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2         OFF CACHE BOOL "" FORCE)
    set(USE_MANUAL          OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2         OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI         OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD           OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP   ON  CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS  ON  CACHE BOOL "" FORCE)
    if(WIN32)
        set(CURL_USE_SCHANNEL    ON  CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL     OFF CACHE BOOL "" FORCE)
        set(CURL_USE_MBEDTLS     OFF CACHE BOOL "" FORCE)
        set(CURL_USE_BEARSSL     OFF CACHE BOOL "" FORCE)
        set(CURL_USE_WOLFSSL     OFF CACHE BOOL "" FORCE)
    else()
        set(CURL_USE_OPENSSL     ON  CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(doki_libcurl)

    # libcurl is a C project; force-disable its PIC requirement so it can
    # link cleanly into our IDA plugin DLL on MSVC (no PIC on x64).
    set_target_properties(libcurl_static PROPERTIES POSITION_INDEPENDENT_CODE OFF)

    set(DOKI_HAS_EMBEDDED_CURL 1)
endif()

# Ensure src/doki/assets.cpp sees DOKI_HAS_EMBEDDED_CURL as 0/1, not a
# generator expression that has not been resolved at the TU level.
set(DOKI_HAS_EMBEDDED_CURL_INT 0)
if(DOKI_USE_EMBEDDED_CURL)
    set(DOKI_HAS_EMBEDDED_CURL_INT 1)
endif()

ida_add_plugin(doki_theme
    TYPE QT
    QT_COMPONENTS Core Gui Widgets
    SOURCES
        ${DOKI_SOURCES}
    DEFINES
        QT_NAMESPACE=QT
        QT_CORE_LIB
        QT_DLL
        QT_GUI_LIB
        QT_THREAD_SUPPORT
        QT_WIDGETS_LIB
        DOKI_HAS_EMBEDDED_CURL=$<BOOL:${DOKI_USE_EMBEDDED_CURL}>
)

target_include_directories(doki_theme PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Link libcurl (statically) when FetchContent is enabled. libcurl's static
# target uses PRIVATE link libraries for the system deps (ws2_32, bcrypt,
# crypt32 on Windows; pthread, dl, m on Linux), so we add them here too
# to ensure they end up on the link line of the plugin DLL.
if(DOKI_USE_EMBEDDED_CURL)
    target_link_libraries(doki_theme PRIVATE CURL::libcurl)
    if(WIN32)
        target_link_libraries(doki_theme PRIVATE ws2_32 bcrypt crypt32)
    else()
        target_link_libraries(doki_theme PRIVATE pthread dl m)
        if(APPLE)
            target_link_libraries(doki_theme PRIVATE
                "-framework Security"
                "-framework CoreFoundation"
                "-framework CoreServices"
                "-framework SystemConfiguration")
        endif()
    endif()
endif()

target_compile_definitions(doki_theme PRIVATE
    DOKI_VERSION="${DOKI_VERSION}"
)

# nlohmann/json is a heavy single header: MSVC needs /bigobj and UTF-8 source.
if(MSVC)
    target_compile_options(doki_theme PRIVATE /bigobj /utf-8)
endif()

# Regenerate the theme catalog from the snapshotted upstream
# definitions. This is a manual step by design: it requires network
# access (for the initial fetch_upstream.ps1) and is not run on every
# `cmake --build`. Run `tools\fetch_upstream.ps1` first to update the
# pinned snapshot, then `tools\generate_theme_catalog.ps1` (or the
# `generate_catalog` target below) to refresh generated/theme_catalog.json.
if(WIN32)
    add_custom_target(generate_catalog
        COMMAND powershell -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_theme_catalog.ps1"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating theme_catalog.json from upstream definitions")

    add_custom_target(fetch_upstream
        COMMAND powershell -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/fetch_upstream.ps1"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Snapshotting doki-master-theme definitions into third_party/")

    # Packaging: stage DLL + catalog + attribution into a distributable zip.
    # By default this re-uses the committed generated/theme_catalog.json
    # so a clean clone can package without first running fetch_upstream.
    # To force regeneration, pass -DOKI_REGEN_CATALOG=ON or invoke
    # `tools\package.ps1 -RegenerateCatalog` directly.
    option(DOKI_REGEN_CATALOG "Regenerate theme_catalog.json during package" OFF)
    set(_pkg_args "-Version" "${DOKI_VERSION}")
    if(DOKI_REGEN_CATALOG)
        list(APPEND _pkg_args "-RegenerateCatalog")
    endif()
    add_custom_target(package
        COMMAND powershell -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/package.ps1"
                ${_pkg_args}
        DEPENDS doki_theme
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Packaging doki-theme-ida v${DOKI_VERSION}")
endif()
