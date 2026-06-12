//----------------------------------------------------------------------------
// Doki Theme - registry implementation.
//----------------------------------------------------------------------------
#include "doki/registry.h"
#include "doki/paths.h"
#include "doki/log.h"

#include <pro.h>
#include <diskio.hpp>

#include <algorithm>

namespace doki
{

// Collects every file path enumerate_files visits.
struct path_collector_t : public file_enumerator_t
{
  std::vector<std::string> files;
  virtual int visit_file(const char *file) override
  {
    files.push_back(file);
    return 0; // keep going
  }
};

size_t ThemeRegistry::load_dir(const std::string &dir)
{
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

const DokiThemeDefinition *ThemeRegistry::get(const std::string &id) const
{
  for ( const auto &t : m_themes )
    if ( t.id == id )
      return &t;
  return nullptr;
}

} // namespace doki
