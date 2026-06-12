//----------------------------------------------------------------------------
// Doki Theme - persisted configuration ($IDAUSR/doki-theme/config.json).
//----------------------------------------------------------------------------
#pragma once

#include <string>

namespace doki
{

struct DokiConfig
{
  std::string selected_id;        // last applied theme id ("" = none)
  bool sticker_enabled = true;    // show the character sticker
  bool wallpaper_enabled = true;  // show the full-listing wallpaper (if any)
  std::string original_theme;     // IDA theme active before doki first applied
};

// Load config; returns defaults if the file is absent or unreadable.
DokiConfig load_config();

// Persist config. Returns false on write failure.
bool save_config(const DokiConfig &cfg);

} // namespace doki
