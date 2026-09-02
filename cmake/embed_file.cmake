# cmake/embed_file.cmake
#
# Script-mode helper: reads INPUT_FILE as raw bytes and writes a C++ header
# declaring a constexpr byte array plus its length under namespace `doki`.
#
# Usage:
#   cmake -D INPUT_FILE=<path> -D OUTPUT_FILE=<path> -D VAR_NAME=<ident> \
#         -P cmake/embed_file.cmake
#
# Produces a header of the form:
#
#   #pragma once
#   #include <cstddef>
#
#   namespace doki {
#       constexpr unsigned char <VAR_NAME>[] = {
#           0xNN, 0xNN, ...
#       };
#       constexpr std::size_t <VAR_NAME>_len = sizeof(<VAR_NAME>);
#   }
#
# Notes:
#   * Pure CMake, no external tools (xxd, python, etc.) required.
#   * std::size_t is used (with <cstddef>) because consumers may transitively
#     include IDA SDK pro.h, which pollutes unqualified `size_t` in some TUs.

if(NOT INPUT_FILE OR NOT OUTPUT_FILE OR NOT VAR_NAME)
    message(FATAL_ERROR
        "Usage: cmake -D INPUT_FILE=... -D OUTPUT_FILE=... -D VAR_NAME=... "
        "-P embed_file.cmake")
endif()

# Defensive: when this script is invoked through the Visual Studio
# generator's custom-command .bat, the -D value can end up with
# surrounding double-quote characters as part of the string. Strip
# any leading/trailing double-quote or backslash so file(EXISTS) and
# file(READ) work regardless of how the parent quoted the argument.
string(REGEX REPLACE "^[\"\\\\]+" "" INPUT_FILE  "${INPUT_FILE}")
string(REGEX REPLACE "[\"\\\\]+$" "" INPUT_FILE  "${INPUT_FILE}")
string(REGEX REPLACE "^[\"\\\\]+" "" OUTPUT_FILE "${OUTPUT_FILE}")
string(REGEX REPLACE "[\"\\\\]+$" "" OUTPUT_FILE "${OUTPUT_FILE}")

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "Input file does not exist: ${INPUT_FILE}")
endif()

# Read the entire file as a flat hex string (two ASCII chars per byte).
file(READ "${INPUT_FILE}" FILE_CONTENT HEX)

# Insert "0x" before every two characters and ", " after.
string(REGEX REPLACE "(..)" "0x\\1, " FILE_CONTENT "${FILE_CONTENT}")

# Strip the trailing ", " left over from the last byte.
string(REGEX REPLACE ", $" "" FILE_CONTENT "${FILE_CONTENT}")

set(HEADER_CONTENT "#pragma once\n")
string(APPEND HEADER_CONTENT "#include <cstddef>\n\n")
string(APPEND HEADER_CONTENT "namespace doki {\n")
string(APPEND HEADER_CONTENT "    constexpr unsigned char ${VAR_NAME}[] = {\n        ${FILE_CONTENT}\n    };\n")
string(APPEND HEADER_CONTENT "    constexpr std::size_t ${VAR_NAME}_len = sizeof(${VAR_NAME});\n")
string(APPEND HEADER_CONTENT "}\n")

# Ensure the output directory exists.
get_filename_component(_out_dir "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_out_dir}")

file(WRITE "${OUTPUT_FILE}" "${HEADER_CONTENT}")
message(STATUS "Generated embedded header: ${OUTPUT_FILE}")
