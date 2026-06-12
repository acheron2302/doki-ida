//----------------------------------------------------------------------------
// Doki Theme - definition parser (nlohmann/json).
//----------------------------------------------------------------------------
#include "doki/theme.h"

#include <fstream>
#include <cctype>
#include <algorithm>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
                         const json &obj)
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

//----------------------------------------------------------------------------
static void parse_sticker(const json &node, DokiSticker *out)
{
  if ( !node.is_object() )
    return;
  if ( node.contains("name") && node["name"].is_string() )
    out->name = node["name"].get<std::string>();
  if ( node.contains("anchor") && node["anchor"].is_string() )
    out->anchor = node["anchor"].get<std::string>();
  if ( node.contains("opacity") && node["opacity"].is_number() )
    out->opacity = node["opacity"].get<int>();
}

// Mirror of parse_sticker for the optional top-level "background" object.
static void parse_background(const json &node, DokiBackground *out)
{
  if ( !node.is_object() )
    return;
  if ( node.contains("name") && node["name"].is_string() )
    out->name = node["name"].get<std::string>();
  if ( node.contains("anchor") && node["anchor"].is_string() )
    out->anchor = node["anchor"].get<std::string>();
  if ( node.contains("opacity") && node["opacity"].is_number() )
    out->opacity = node["opacity"].get<int>();
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

  json j;
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
    def.dark        = j.value("dark", true);

    if ( j.contains("stickers") && j["stickers"].is_object() )
    {
      const json &st = j["stickers"];
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
    if ( j.contains("overrides")
      && j["overrides"].is_object()
      && j["overrides"].contains("editorScheme")
      && j["overrides"]["editorScheme"].is_object()
      && j["overrides"]["editorScheme"].contains("colors") )
    {
      merge_colors(&def.colors, j["overrides"]["editorScheme"]["colors"]);
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

} // namespace doki
