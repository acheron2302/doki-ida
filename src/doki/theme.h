//----------------------------------------------------------------------------
// Doki Theme - data model for a single character theme.
//
// Mirrors the upstream "*.master.definition.json" schema
// (doki-theme/doki-master-theme). We consume the same files; the engine is
// re-implemented in C++.
//
// Runtime loading is catalog-driven (see registry.cpp): the generator
// (tools/generate_theme_catalog.ps1) walks the upstream
// definitions/ tree, derives sticker and wallpaper CDN remote paths
// from the directory hierarchy, and emits
// generated/theme_catalog.json. The runtime then loads that catalog.
// Each catalog entry already carries fully-resolved remotePath fields
// for stickers (default + secondary) and background, so this parser
// does not need a hard-coded lookup table.
//----------------------------------------------------------------------------
#pragma once

// IMPORTANT: do NOT include <nlohmann/json.hpp> in this header.
//
// The IDA SDK's pro.h #defines snprintf to dont_use_snprintf and
// similar legacy macro pollution breaks nlohmann/json's templates
// the moment they are instantiated. Putting nlohmann/json in this
// public header would force every TU that includes theme.h
// (plugin.cpp, ui.cpp) to also include pro.h, and the nlohmann
// templates would then be re-parsed against the polluted namespace.
//
// The nlohmann::json type is only needed at call sites: theme.cpp
// and registry.cpp. Both of them include <nlohmann/json.hpp> first,
// before any IDA SDK header, and never see the polluted namespace.
//
// To keep theme.h free of nlohmann/json, the JSON-based entry
// point uses a serialized form: load_definition_from_json_string
// takes the raw JSON text. Callers that already have a parsed
// nlohmann::json (registry.cpp) call nlohmann::json::dump() to
// serialize, and nlohmann::json::parse() to re-parse after the
// call. This is slightly wasteful but it keeps the public header
// free of nlohmann/json and works around the macro-pollution
// problem documented above.

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
  std::string remote_path; // CDN-relative path (e.g. "backgrounds/wallpapers/transparent/darkness_dark.png")
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
  std::string product;     // "community" or "ultimate"
  bool dark = true;

  DokiSticker sticker;     // primary sticker (stickers.default)
  DokiSticker secondary;   // optional (stickers.secondary)

  DokiBackground background; // optional full-listing wallpaper (top-level "background")

  // Effective palette: base "colors" with "overrides.editorScheme.colors"
  // merged on top (the editor override wins, since IDA's listing *is* the
  // editor surface). Values are normalized lowercase "#rrggbb" / "#rrggbbaa".
  std::map<std::string, std::string> colors;

  // Raw base "colors.accentColor" captured BEFORE
  // "overrides.editorScheme.colors" is merged on top. This is the upstream
  // contract: the chrome surface (Qt widgets, tooltips, etc.) uses the base
  // accent, while the editor/listing surface uses the override. Empty when
  // the definition omits the base accent. When set, it is a normalized
  // lowercase hex string (e.g. "#ececec"); map_theme() promotes it into
  // ui_accent / accent.
  std::string chrome_accent_color;

  // Raw "overrides.editorScheme.colors.accentColor" captured BEFORE the
  // override merge overwrites it. This is the upstream contract: the editor
  // surface (listing/decompiler) gets its own accent distinct from the chrome
  // accent. Empty when the definition has no editor-scheme override or when
  // it omits accentColor there. When set, it is a normalized lowercase hex
  // string (e.g. "#7ceec8"); map_theme() promotes it into editor_accent.
  std::string editor_accent_color;

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
// Used for legacy per-file definitions only; the runtime loads the
// generated catalog via load_definition_from_json() instead.
bool load_definition(const char *path,
                     DokiThemeDefinition *out,
                     std::string *err);

} // namespace doki

namespace doki
{

// Parse one definition from a serialized JSON string. The shape is
// either a master definition (top-level "colors", "stickers", etc.)
// or a catalog entry (already includes fully-resolved "stickers.*.remotePath"
// and "background.remotePath"). Sticker and background remote paths
// are read directly from the JSON when present; the legacy hard-coded
// path lookup has been removed. On failure returns false and fills *err.
//
// This string-based entry point exists so the doki/theme.h header can
// stay free of <nlohmann/json.hpp>. The catalog loader, which already
// has a parsed nlohmann::json object, calls nlohmann::json::dump() to
// get the text, then this function. See the long comment at the top
// of this header for the macro-pollution reason.
bool load_definition_from_json_string(const std::string &json_text,
                                      DokiThemeDefinition *out,
                                      std::string *err);

} // namespace doki
