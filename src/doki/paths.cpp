//----------------------------------------------------------------------------
// Doki Theme - path resolution implementation.
//----------------------------------------------------------------------------
#include "doki/paths.h"

#include <pro.h>
#include <diskio.hpp>

namespace doki
{

#ifdef __NT__
static const char SEP = '\\';
#else
static const char SEP = '/';
#endif

std::string path_join(const std::string &a, const std::string &b)
{
  if ( a.empty() )
    return b;
  if ( b.empty() )
    return a;
  if ( a.back() == SEP || a.back() == '/' )
    return a + b;
  return a + SEP + b;
}

std::string user_idadir()
{
  const char *d = get_user_idadir();
  return d != nullptr ? std::string(d) : std::string();
}

std::string doki_root()        { return path_join(user_idadir(), "doki-theme"); }
std::string definitions_dir()  { return path_join(doki_root(), "definitions"); }
std::string assets_dir()       { return path_join(doki_root(), "assets"); }
std::string catalog_path()     { return path_join(doki_root(), "theme_catalog.json"); }
std::string themes_dir()       { return path_join(user_idadir(), "themes"); }

bool ensure_dir(const std::string &dir)
{
  if ( dir.empty() )
    return false;
  if ( qisdir(dir.c_str()) )
    return true;

  // Create parents top-down.
  std::string cur;
  for ( size_t i = 0; i < dir.size(); ++i )
  {
    char c = dir[i];
    cur.push_back(c);
    bool at_sep = (c == SEP || c == '/');
    bool at_end = (i + 1 == dir.size());
    if ( (at_sep || at_end) && cur.size() > 1 )
    {
      std::string part = at_sep ? cur.substr(0, cur.size() - 1) : cur;
      if ( !part.empty() && !qisdir(part.c_str()) )
        qmkdir(part.c_str(), 0755);
    }
  }
  return qisdir(dir.c_str());
}

} // namespace doki
