//----------------------------------------------------------------------------
// Doki Theme - on-demand asset manager.
//
// Downloads Doki sticker / wallpaper PNGs from the project's CDN
// (https://doki.assets.unthrottled.io) on first use, caches them under
// $IDAUSR/doki-theme/cache/{stickers,wallpapers}/<file>, and returns the
// cached path. The cache is the local file name (e.g. "rem.png"); the
// remote layout is opaque to callers.
//
// The manager is best-effort by design: failures (network, HTTP, decode)
// log a warning and return an empty path so the caller's artwork install
// degrades gracefully (colors / nav styling still apply). It is also
// thread-aware enough to guard against re-entrant fetches within a single
// apply pass via a static in-flight set; the Qt request itself runs on
// the calling thread with a short bounded timeout so Qt events keep
// processing.
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace doki
{

// Asset category for the CDN cache and remote path validation.
enum class AssetKind
{
  Sticker,   // remote path must start with "stickers/"
  Wallpaper  // remote path must start with "backgrounds/" (the
             // transparent variant at "backgrounds/wallpapers/transparent/"
             // is what the bundled themes use so the code listing and
             // doki palette show through wherever the artwork is empty)
};

// Validate a definition-supplied remote path for the given category.
// Returns true iff:
//   * the path is non-empty,
//   * uses forward slashes only (no backslashes),
//   * contains no ".." segments,
//   * has the expected prefix ("stickers/" or "backgrounds/"),
//   * and ends with a plausible file name.
//
// The function does NOT check that the path resolves to the configured CDN
// host; the URL builder in fetch_asset() is the only path-construction site
// and is hard-coded to https://doki.assets.unthrottled.io.
bool is_valid_remote_path(AssetKind kind, const std::string &remote_path);

// Build the absolute URL for a validated remote path. The host and scheme
// are hard-coded. Asserts (well, returns empty) for invalid input.
std::string cdn_url_for(AssetKind kind, const std::string &remote_path);

// Local cache directory for a given category
//   $IDAUSR/doki-theme/cache/stickers
//   $IDAUSR/doki-theme/cache/wallpapers
std::string cache_dir(AssetKind kind);

// Local cache file path for a (kind, file_name) pair.
std::string cache_file_path(AssetKind kind, const std::string &file_name);

// Ensure the asset for (kind, file_name, remote_path) is available locally:
//   1. If a valid cache file already exists, return its path.
//   2. Otherwise, attempt a bounded download from the CDN.
//   3. On success, validate the PNG and atomically promote a temp file
//      into the cache, then return the path.
//   4. On any failure (network, HTTP, decode, I/O), log a concise warning,
//      remove any temp file, and return an empty string.
//
// The function is safe to call from the main thread while IDA is running:
// it uses Qt's nested event loop with a strict timeout. Multiple concurrent
// callers for the same (kind, file_name) are coalesced.
std::string ensure_asset(AssetKind kind,
                         const std::string &file_name,
                         const std::string &remote_path);

} // namespace doki
