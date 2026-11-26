//----------------------------------------------------------------------------
// Doki Theme - UTF-8 aware std::filesystem path conversion.
//
// IDA returns UTF-8 paths (e.g. $IDAUSR with a non-ASCII user name), while
// MSVC's narrow std::ifstream/std::ofstream constructors interpret char*
// paths in the active ANSI code page. Convert through filesystem::path so
// the wide Win32 APIs are used; POSIX paths are UTF-8 byte strings already.
//
// Include this header before any IDA SDK header: the SDK's pro.h macro
// pollution (snprintf/fopen/...) must not be active while <filesystem> is
// being parsed.
//----------------------------------------------------------------------------
#pragma once

#include <filesystem>
#include <string>

namespace doki
{

inline std::filesystem::path fs_path(const std::string &path)
{
#if defined(_WIN32)
  // IDA normally returns UTF-8, but paths derived from environment variables
  // can arrive in the active ANSI code page. u8path() rejects the latter
  // with "No mapping for the Unicode character..." so fall back to the
  // native narrow conversion (which uses ACP on Windows).
  try
  {
    return std::filesystem::u8path(path);
  }
  catch ( const std::exception & )
  {
    return std::filesystem::path(path);
  }
#else
  return std::filesystem::path(path);
#endif
}

} // namespace doki
