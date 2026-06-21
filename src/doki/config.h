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
  // false means the generated CSS/IDA default nav colors are used and Doki
  // does not install the runtime SDK nav-band gradient colorizer. The nav
  // band then reflects the theme.css `navband_t` block colors instead of
  // the live doki gradient.
  bool live_nav_colorizer_enabled = false;
};

// Load config; returns defaults if the file is absent or unreadable.
DokiConfig load_config();

// Persist config. Returns false on write failure.
bool save_config(const DokiConfig &cfg);

} // namespace doki
