//----------------------------------------------------------------------------
// Doki Theme - runtime path resolution. No hardcoded absolute paths: every
// location is derived from $IDAUSR via the SDK.
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace doki
{

// $IDAUSR (e.g. %APPDATA%\Hex-Rays\IDA Pro on Windows).
std::string user_idadir();

// <IDAUSR>/doki-theme  -- our asset/config root.
std::string doki_root();

// <IDAUSR>/doki-theme/definitions  -- bundled theme definitions.
std::string definitions_dir();

// <IDAUSR>/doki-theme/theme_catalog.json  -- generated catalog of all
// official doki themes. This is the runtime source of truth.
std::string catalog_path();

// <IDAUSR>/doki-theme/assets  -- bundled stickers/backgrounds.
std::string assets_dir();

// <IDAUSR>/themes  -- where generated IDA themes are installed.
std::string themes_dir();

// Join two path components with the platform separator.
std::string path_join(const std::string &a, const std::string &b);

// True for a single, non-traversing file name (no separators, no "..").
// Used before suffixing definition-supplied asset names to a directory.
bool is_safe_leaf_name(const std::string &name);

// Directory existence check using the same UTF-8/ANSI path conversion as
// ensure_dir(). Do not use qisdir() for runtime paths on Windows.
bool is_dir(const std::string &dir);

// Create a directory (and parents). Returns true on success or if it exists.
bool ensure_dir(const std::string &dir);

} // namespace doki
