//----------------------------------------------------------------------------
// Doki Theme - definition parser (nlohmann/json).
//
// Two entry points:
//   * load_definition()               -> reads a JSON file (legacy /
//     optional user overrides).
//   * load_definition_from_json()     -> reads an in-memory JSON object
//     (used by the catalog loader; also keeps a single parsing path
//     for legacy files).
//
// Remote paths for stickers and the background are read directly from
// the JSON. The earlier hard-coded CDN-path lookup table has been
// removed; the catalog generator (tools/generate_theme_catalog.ps1)
// is the single source of truth for upstream CDN paths. If a
// catalog entry or a custom definition omits a remotePath the
// resulting field is simply empty and the asset manager will log a
// warning at apply time.
//
// nlohmann/json is included first, before any header that might transitively
// include IDA SDK macro pollution. See theme.h/registry.cpp for details.
//----------------------------------------------------------------------------
#include <nlohmann/json.hpp>

#include "doki/theme.h"

#include <fstream>
#include <cctype>
#include <algorithm>

namespace doki
{

//----------------------------------------------------------------------------
static std::string to_lower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

// Normalize a color literal. Hex stays as lowercase "#......"; anything else
// (e.g. a named reference) is returned unchanged for the resolver to handle.
static std::string normalize_color(const std::string &raw)
{
  if ( !raw.empty() && raw[0] == '#' )
    return to_lower(raw);
  return raw;
}

// Merge a json object of {key: colorString} into dst (overwriting).
static void merge_colors(std::map<std::string, std::string> *dst,
                         const nlohmann::json &obj)
{
  if ( !obj.is_object() )
    return;
  for ( auto it = obj.begin(); it != obj.end(); ++it )
  {
    if ( it.value().is_string() )
      (*dst)[it.key()] = normalize_color(it.value().get<std::string>());
  }
}

// Resolve intra-palette references: a value that is not a hex literal but
// names another key is replaced by that key's value (one hop, cycle-guarded).
static void resolve_references(std::map<std::string, std::string> *colors)
{
  for ( auto &kv : *colors )
  {
    std::string seen_start = kv.first;
    int hops = 0;
    while ( !kv.second.empty() && kv.second[0] != '#' && hops < 8 )
    {
      auto ref = colors->find(kv.second);
      if ( ref == colors->end() || ref->first == kv.first )
        break;
      kv.second = ref->second;
      ++hops;
    }
  }
}

// Populate a sticker / background from a JSON object. Reads name, anchor,
// and remotePath directly. remotePath is intentionally NOT derived from
// a hard-coded lookup table anymore; the catalog generator is the
// authoritative source of those paths.
static void parse_sticker(const nlohmann::json &node, DokiSticker *out)
{
  if ( !node.is_object() )
    return;
  if ( node.contains("name") && node["name"].is_string() )
    out->name = node["name"].get<std::string>();
  if ( node.contains("anchor") && node["anchor"].is_string() )
    out->anchor = node["anchor"].get<std::string>();
  if ( node.contains("remotePath") && node["remotePath"].is_string() )
    out->remote_path = node["remotePath"].get<std::string>();
}

// Mirror of parse_sticker for the optional top-level "background" object.
static void parse_background(const nlohmann::json &node, DokiBackground *out)
{
  if ( !node.is_object() )
    return;
  if ( node.contains("name") && node["name"].is_string() )
    out->name = node["name"].get<std::string>();
  if ( node.contains("anchor") && node["anchor"].is_string() )
    out->anchor = node["anchor"].get<std::string>();
  if ( node.contains("remotePath") && node["remotePath"].is_string() )
    out->remote_path = node["remotePath"].get<std::string>();
}

//----------------------------------------------------------------------------
static bool load_definition_from_json_internal(const nlohmann::json &j,
                                               DokiThemeDefinition *out,
                                               std::string *err)
{
  if ( !j.is_object() )
  {
    if ( err != nullptr )
      *err = "top-level JSON is not an object";
    return false;
  }

  DokiThemeDefinition def;
  try
  {
    def.id          = j.value("id", std::string());
    def.name        = j.value("name", std::string());
    def.displayName = j.value("displayName", def.name);
    def.group       = j.value("group", std::string());
    def.author      = j.value("author", std::string());
    def.product     = j.value("product", std::string("community"));
    def.dark        = j.value("dark", true);

    if ( j.contains("stickers") && j["stickers"].is_object() )
    {
      const nlohmann::json &st = j["stickers"];
      if ( st.contains("default") )
        parse_sticker(st["default"], &def.sticker);
      if ( st.contains("secondary") )
        parse_sticker(st["secondary"], &def.secondary);
    }

    if ( j.contains("background") )
      parse_background(j["background"], &def.background);

    // Base palette, then editor-scheme overrides on top.
    if ( j.contains("colors") )
      merge_colors(&def.colors, j["colors"]);

    // Capture the base accentColor BEFORE the editor-scheme override merge
    // overwrites it. The chrome (Qt / window chrome) surface uses the base
    // accent, while the listing/decompiler surface uses the override. After
    // the merge, def.colors["accentColor"] is the overridden value, so this
    // pre-merge snapshot is the only way to recover the chrome accent.
    auto base_acc = def.colors.find("accentColor");
    if ( base_acc != def.colors.end() )
      def.chrome_accent_color = base_acc->second;

    if ( j.contains("overrides")
      && j["overrides"].is_object()
      && j["overrides"].contains("editorScheme")
      && j["overrides"]["editorScheme"].is_object()
      && j["overrides"]["editorScheme"].contains("colors") )
    {
      const nlohmann::json &ec = j["overrides"]["editorScheme"]["colors"];
      // Capture the editor-scheme accent BEFORE the merge so callers can
      // distinguish "editor accent" from "chrome accent". After the merge,
      // def.colors["accentColor"] is the overridden (mint) value, so the
      // pre-merge value would otherwise be lost.
      if ( ec.is_object() && ec.contains("accentColor")
        && ec["accentColor"].is_string() )
      {
        def.editor_accent_color = normalize_color(
            ec["accentColor"].get<std::string>());
      }
      merge_colors(&def.colors, ec);
    }

    resolve_references(&def.colors);
  }
  catch ( const std::exception &e )
  {
    if ( err != nullptr )
      *err = std::string("schema error: ") + e.what();
    return false;
  }

  // Minimal validity: a usable theme needs at least a background + foreground.
  if ( def.id.empty() || !def.has_color("baseBackground") )
  {
    if ( err != nullptr )
      *err = "missing required fields (id / baseBackground)";
    return false;
  }

  *out = std::move(def);
  return true;
}

bool load_definition_from_json_string(const std::string &json_text,
                                      DokiThemeDefinition *out,
                                      std::string *err)
{
  nlohmann::json j;
  try
  {
    j = nlohmann::json::parse(json_text);
  }
  catch ( const std::exception &e )
  {
    if ( err != nullptr )
      *err = std::string("JSON parse error: ") + e.what();
    return false;
  }
  return load_definition_from_json_internal(j, out, err);
}

//----------------------------------------------------------------------------
bool load_definition(const char *path,
                     DokiThemeDefinition *out,
                     std::string *err)
{
  std::ifstream ifs(path, std::ios::binary);
  if ( !ifs )
  {
    if ( err != nullptr )
      *err = "cannot open file";
    return false;
  }

  nlohmann::json j;
  try
  {
    ifs >> j;
  }
  catch ( const std::exception &e )
  {
    if ( err != nullptr )
      *err = std::string("JSON parse error: ") + e.what();
    return false;
  }

  return load_definition_from_json_internal(j, out, err);
}

} // namespace doki
