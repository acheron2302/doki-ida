//----------------------------------------------------------------------------
// Doki Theme - registry implementation.
//----------------------------------------------------------------------------
// nlohmann/json must be included BEFORE any IDA SDK header because
// the SDK's pro.h #defines snprintf, _strtoui64, and others, which
// break nlohmann's internals. The forward declaration in
// doki/theme.h is enough for the registry.h signature; we explicitly
// include the full header here, before pro.h, so its template
// declarations are parsed against an unpolluted std namespace.
#include <nlohmann/json.hpp>

#include "doki/registry.h"
#include "doki/paths.h"
#include "doki/log.h"

#include <pro.h>
#include <diskio.hpp>

#include <algorithm>
#include <fstream>

namespace doki
{

// nlohmann::json is now a complete type via the include above.

// Local file_enumerator_t used by load_dir. The struct is private to
// this translation unit because it relies on diskio.hpp (which pulls
// in the IDA SDK pro.h macro zoo, and would not be safe to include
// from a public header). Callers that need to enumerate the same
// directory should declare their own file_enumerator_t.
struct path_collector_t : public file_enumerator_t
{
  std::vector<std::string> files;
  int visit_file(const char *file) override
  {
    files.push_back(file);
    return 0; // keep going
  }
};

// ----- load_dir -------------------------------------------------------------
size_t ThemeRegistry::load_dir(const std::string &dir)
{
  // For load_dir() we want a *replacement* semantics: callers using
  // load_dir() (custom-definitions, tests) expect a clean slate.
  // load_catalog() also replaces; combined catalog+overrides flow
  // should call load_catalog() then iterate the override dir
  // externally.
  m_themes.clear();

  if ( dir.empty() || !qisdir(dir.c_str()) )
  {
    doki::msg_log("definitions dir not found: %s\n", dir.c_str());
    return 0;
  }

  path_collector_t collector;
  char answer[QMAXPATH];
  enumerate_files(answer, sizeof(answer), dir.c_str(), "*.json", collector);

  // Deterministic order regardless of filesystem enumeration.
  std::sort(collector.files.begin(), collector.files.end());

  size_t ok = 0, bad = 0;
  for ( const std::string &path : collector.files )
  {
    DokiThemeDefinition def;
    std::string err;
    if ( load_definition(path.c_str(), &def, &err) )
    {
      m_themes.push_back(std::move(def));
      ++ok;
    }
    else
    {
      ++bad;
      doki::msg_log("skipping '%s': %s\n", path.c_str(), err.c_str());
    }
  }

  doki::msg_log("loaded %u theme(s) from %s (%u skipped)\n",
                (uint)ok, dir.c_str(), (uint)bad);
  return ok;
}

// ----- load_catalog ---------------------------------------------------------
size_t ThemeRegistry::load_catalog(const std::string &path)
{
  m_themes.clear();

  std::ifstream ifs(path, std::ios::binary);
  if ( !ifs )
  {
    doki::msg_log("catalog not found: %s\n", path.c_str());
    return 0;
  }

  nlohmann::json j;
  try
  {
    ifs >> j;
  }
  catch ( const std::exception &e )
  {
    doki::msg_log("catalog: JSON parse error: %s\n", e.what());
    return 0;
  }

  if ( !j.is_object() )
  {
    doki::msg_log("catalog: top-level JSON is not an object\n");
    return 0;
  }

  // Schema check. We only support schemaVersion 1.
  int schema = j.value("source", nlohmann::json::object()).value("schemaVersion", 0);
  if ( schema != 1 )
  {
    doki::msg_log("catalog: unsupported schemaVersion %d (expected 1)\n", schema);
    return 0;
  }

  const nlohmann::json &themes = j["themes"];
  if ( !themes.is_array() )
  {
    doki::msg_log("catalog: 'themes' is not an array\n");
    return 0;
  }

  // Themes array is already sorted by the generator; we re-sort here
  // defensively so the registry is correct even if the file is hand-
  // edited. We re-serialize each entry and call the string-based
  // parser, so doki/theme.h does not need to expose nlohmann::json
  // in its public API (see the long comment in theme.h).
  std::vector<DokiThemeDefinition> parsed;
  parsed.reserve(themes.size());
  for ( const auto &t : themes )
  {
    DokiThemeDefinition def;
    std::string err;
    const std::string entry_text = t.dump();
    if ( load_definition_from_json_string(entry_text, &def, &err) )
    {
      parsed.push_back(std::move(def));
    }
    else
    {
      doki::msg_log("catalog: skipping theme (%s)\n", err.c_str());
    }
  }

  std::sort(parsed.begin(), parsed.end(),
            [](const DokiThemeDefinition &a, const DokiThemeDefinition &b)
            {
              int c = a.group.compare(b.group);
              if ( c != 0 ) return c < 0;
              c = a.displayName.compare(b.displayName);
              if ( c != 0 ) return c < 0;
              c = a.name.compare(b.name);
              if ( c != 0 ) return c < 0;
              return a.id < b.id;
            });

  size_t ok = parsed.size();
  size_t bad = themes.size() - ok;
  m_themes = std::move(parsed);

  doki::msg_log("loaded %u theme(s) from catalog %s (%u skipped)\n",
                (uint)ok, path.c_str(), (uint)bad);
  return ok;
}

// ----- get ------------------------------------------------------------------
const DokiThemeDefinition *ThemeRegistry::get(const std::string &id) const
{
  for ( const auto &t : m_themes )
    if ( t.id == id )
      return &t;
  return nullptr;
}

// ----- upsert ---------------------------------------------------------------
bool ThemeRegistry::upsert(const DokiThemeDefinition &t)
{
  for ( auto &existing : m_themes )
  {
    if ( existing.id == t.id )
    {
      existing = t;
      return true;
    }
  }
  m_themes.push_back(t);
  return false;
}

} // namespace doki
