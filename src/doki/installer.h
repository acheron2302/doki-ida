//----------------------------------------------------------------------------
// Doki Theme - installs a definition as a real IDA theme folder and activates
// it. A theme is materialized under $IDAUSR/themes/<theme_name>/ containing a
// generated theme.css plus the sticker image (so the CSS $RELPATH resolves).
// Activation sets HKCU\..\IDA\ThemeName, which IDA reads to pick the theme.
//----------------------------------------------------------------------------
#pragma once

#include <string>
#include "doki/theme.h"

namespace doki
{

struct InstallResult
{
  bool ok = false;
  std::string theme_name;   // e.g. "doki-zero-two-light-sakura"
  std::string css_path;     // full path to the written theme.css
  bool sticker_installed = false;
  bool wallpaper_installed = false;
  std::string err;
};

// Folder/theme name for a definition (stable, filesystem-safe).
std::string theme_name_for(const DokiThemeDefinition &def);

// Write themes/<name>/{theme.css, sticker.png} and activate the theme.
// 'activate'      controls whether the registry ThemeName is updated.
// 'with_sticker'  controls whether the character sticker is emitted/copied.
// 'with_wallpaper' controls whether a full-listing wallpaper is used (only if
//                  a matching wallpaper asset exists for this theme).
InstallResult install_theme(const DokiThemeDefinition &def,
                            bool activate = true,
                            bool with_sticker = true,
                            bool with_wallpaper = true);

// Set the active IDA theme by folder name (registry ThemeName).
void activate_theme(const std::string &theme_name);

// Current active theme name (empty if unreadable).
std::string current_theme();

} // namespace doki
