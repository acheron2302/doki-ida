//----------------------------------------------------------------------------
// Doki Theme - path resolution implementation.
//----------------------------------------------------------------------------
#include "doki/paths.h"
#include "doki/fs.h"

#include <cctype>
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
  if ( d == nullptr )
    return std::string();
#if defined(_WIN32)
  // IDA can return this path as UTF-8 (normal) or in the active ANSI code
  // page (when it originated from an environment variable). Normalize to
  // UTF-8 so every downstream std::string / QString path is consistent.
  try
  {
    return std::filesystem::u8path(d).u8string();
  }
  catch ( const std::exception & )
  {
    return std::filesystem::path(d).u8string();
  }
#else
  return std::string(d);
#endif
}

bool is_safe_leaf_name(const std::string &name)
{
  if ( name.empty() || name == "." || name == ".." )
    return false;
  if ( name.find('/') != std::string::npos
    || name.find('\\') != std::string::npos )
    return false;
  // Reject characters Windows cannot represent in a file name and control
  // bytes. This also prevents alternate-data-stream writes via ':'.
  for ( unsigned char c : name )
  {
    if ( c < 0x20
      || c == ':' || c == '*' || c == '?' || c == '"'
      || c == '<' || c == '>' || c == '|' )
      return false;
  }
  if ( name.back() == ' ' || name.back() == '.' )
    return false;
  // Windows reserved device names, with or without an extension.
  std::string base = name.substr(0, name.find('.'));
  for ( char &c : base )
    c = (char)std::tolower((unsigned char)c);
  if ( base == "con" || base == "prn" || base == "aux" || base == "nul" )
    return false;
  if ( base.size() == 4
    && (base.compare(0, 3, "com") == 0 || base.compare(0, 3, "lpt") == 0)
    && base[3] >= '1' && base[3] <= '9' )
    return false;
  return true;
}

bool is_dir(const std::string &dir)
{
  if ( dir.empty() )
    return false;
  std::error_code ec;
  return std::filesystem::is_directory(fs_path(dir), ec);
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

  // filesystem handles parent creation and Windows UNC paths in one step,
  // and its path conversion handles both UTF-8 and IDA's ANSI fallback.
  // Do not verify with qisdir() here: qisdir interprets the raw bytes as
  // UTF-8 and would reject an ANSI path that create_directories just made.
  std::error_code ec;
  const std::filesystem::path p = fs_path(dir);
  if ( std::filesystem::is_directory(p, ec) )
    return true;
  std::filesystem::create_directories(p, ec);
  return std::filesystem::is_directory(p, ec);
}

} // namespace doki
