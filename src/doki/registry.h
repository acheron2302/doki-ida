//----------------------------------------------------------------------------
// Doki Theme - registry of all available character themes.
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
  // Scan dir for "*.master.definition.json" (and "*.json") and load each.
  // Malformed files are skipped with a logged warning. Returns the number of
  // themes successfully loaded. Clears any previously loaded themes first.
  size_t load_dir(const std::string &dir);

  const std::vector<DokiThemeDefinition> &themes() const { return m_themes; }
  size_t size() const { return m_themes.size(); }
  bool empty() const { return m_themes.empty(); }

  // Lookup by GUID; nullptr if not found.
  const DokiThemeDefinition *get(const std::string &id) const;

private:
  std::vector<DokiThemeDefinition> m_themes;
};

} // namespace doki
