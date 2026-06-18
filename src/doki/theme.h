//----------------------------------------------------------------------------
// Doki Theme - data model for a single character theme.
//
// Mirrors the upstream "*.master.definition.json" schema
// (doki-theme/doki-master-theme). We consume the same files; the engine is
// re-implemented in C++.
//----------------------------------------------------------------------------
#pragma once

#include <string>
#include <map>

namespace doki
{

// A character sticker (corner artwork).
struct DokiSticker
{
  std::string name;        // local cache file name, e.g. "rem.png"
  std::string remote_path; // CDN-relative path (e.g. "stickers/vscode/reZero/rem/rem.png")
  std::string anchor;      // "center", "right", "left", ...
  bool valid() const { return !name.empty(); }
};

// A full-listing background / wallpaper. Same shape as DokiSticker so the
// JSON schema is uniform. anchor is honored when emitted.
struct DokiBackground
{
  std::string name;        // local cache file name, e.g. "darkness_dark.png"
  std::string remote_path; // CDN-relative path (e.g. "backgrounds/darkness_dark.png")
  std::string anchor;      // "center", "right", "left", ...
  bool valid() const { return !name.empty(); }
};

// One fully-parsed doki theme definition.
struct DokiThemeDefinition
{
  std::string id;          // GUID
  std::string name;        // internal name ("Zero Two Light Sakura")
  std::string displayName; // shown to the user ("Zero Two")
  std::string group;       // series ("Darling in the Franxx")
  std::string author;
  bool dark = true;

  DokiSticker sticker;     // primary sticker (stickers.default)
  DokiSticker secondary;   // optional (stickers.secondary)

  DokiBackground background; // optional full-listing wallpaper (top-level "background")

  // Effective palette: base "colors" with "overrides.editorScheme.colors"
  // merged on top (the editor override wins, since IDA's listing *is* the
  // editor surface). Values are normalized lowercase "#rrggbb" / "#rrggbbaa".
  std::map<std::string, std::string> colors;

  bool has_color(const std::string &key) const
  {
    return colors.find(key) != colors.end();
  }

  // Returns the color for key, or fallback if absent.
  std::string color(const std::string &key,
                    const std::string &fallback = std::string()) const
  {
    auto it = colors.find(key);
    return it == colors.end() ? fallback : it->second;
  }
};

// Parse one definition file. On failure returns false and fills *err.
bool load_definition(const char *path,
                     DokiThemeDefinition *out,
                     std::string *err);

} // namespace doki
