//----------------------------------------------------------------------------
// Doki Theme - registry of all available character themes.
//
// The primary source is the generated catalog
// (generated/theme_catalog.json), shipped alongside the plugin and
// copied to $IDAUSR/doki-theme/theme_catalog.json on deploy. The
// catalog carries the upstream CDN remote paths for stickers and
// wallpapers, so the registry does not need to derive them itself.
//
// load_dir() remains for two narrow use cases:
//   1) Loading optional user-supplied custom definitions dropped into
//      $IDAUSR/doki-theme/definitions/. These are loaded *after* the
//      catalog and can override catalog entries by id.
//   2) Loading the bundled hand-maintained definitions during
//      self-tests on machines where the generated catalog has not been
//      deployed yet.
//----------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

#include "doki/theme.h"

namespace doki
{

class ThemeRegistry
{
public:
  // Load generated/theme_catalog.json. Requires schemaVersion == 1.
  // Returns the number of themes successfully loaded. Malformed
  // entries are skipped with a logged warning. Clears any previously
  // loaded themes first.
  size_t load_catalog(const std::string &path);

  // Scan dir for "*.master.definition.json" (and "*.json") and load
  // each. Used for user-supplied custom definitions and for tests.
  // Malformed files are skipped with a logged warning. Returns the
  // number of themes successfully loaded. Existing themes are
  // replaced (catalog re-loads should call load_catalog instead).
  size_t load_dir(const std::string &dir);

  // Append a single already-parsed theme definition. Used to layer
  // user custom definitions on top of the catalog without re-reading
  // the catalog from disk.
  void append(const DokiThemeDefinition &t) { m_themes.push_back(t); }

  // Insert or replace a theme by id. If a theme with the same id
  // already exists, it is replaced in-place (so user-supplied custom
  // definitions dropped into $IDAUSR/doki-theme/definitions/ actually
  // *replace* catalog entries with the same id, not just append another
  // copy that the picker hides behind the first one). When no such
  // theme exists, the new definition is appended. Returns true if an
  // existing entry was replaced, false if the theme was appended.
  bool upsert(const DokiThemeDefinition &t);

  const std::vector<DokiThemeDefinition> &themes() const { return m_themes; }
  size_t size() const { return m_themes.size(); }
  bool empty() const { return m_themes.empty(); }

  // Lookup by GUID; nullptr if not found.
  const DokiThemeDefinition *get(const std::string &id) const;

private:
  std::vector<DokiThemeDefinition> m_themes;
};

} // namespace doki
