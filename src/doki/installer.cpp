//----------------------------------------------------------------------------
// Doki Theme - installer implementation.
//----------------------------------------------------------------------------
#include "doki/installer.h"
#include "doki/paths.h"
#include "doki/palette.h"
#include "doki/css.h"
#include "doki/log.h"

#include <pro.h>
#include <registry.hpp>

#include <fstream>
#include <cctype>

namespace doki
{

// "Zero Two Light Sakura" -> "doki-zero-two-light-sakura"
std::string theme_name_for(const DokiThemeDefinition &def)
{
  const std::string &src = !def.name.empty() ? def.name : def.id;
  std::string slug;
  bool prev_dash = false;
  for ( char c : src )
  {
    unsigned char u = (unsigned char)c;
    if ( std::isalnum(u) )
    {
      slug.push_back((char)std::tolower(u));
      prev_dash = false;
    }
    else if ( !prev_dash )
    {
      slug.push_back('-');
      prev_dash = true;
    }
  }
  while ( !slug.empty() && slug.back() == '-' )
    slug.pop_back();
  return "doki-" + slug;
}

static bool write_text_file(const std::string &path, const std::string &content)
{
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  if ( !ofs )
    return false;
  ofs.write(content.data(), (std::streamsize)content.size());
  return (bool)ofs;
}

static bool copy_binary_file(const std::string &src, const std::string &dst)
{
  std::ifstream in(src, std::ios::binary);
  if ( !in )
    return false;
  std::ofstream out(dst, std::ios::binary | std::ios::trunc);
  if ( !out )
    return false;
  out << in.rdbuf();
  return (bool)out && (bool)in;
}

void activate_theme(const std::string &theme_name)
{
  reg_write_string("ThemeName", theme_name.c_str());
}

std::string current_theme()
{
  qstring v;
  if ( reg_read_string(&v, "ThemeName") )
    return v.c_str();
  return std::string();
}

// Installed wallpaper file name inside a theme folder (fixed, so it never
// collides with a sticker that happens to share the source file name).
static const char *const WALLPAPER_FILE = "doki_wallpaper.png";

InstallResult install_theme(const DokiThemeDefinition &def, bool activate,
                            bool with_sticker, bool with_wallpaper)
{
  InstallResult r;
  r.theme_name = theme_name_for(def);

  const std::string dir = path_join(themes_dir(), r.theme_name);
  if ( !ensure_dir(dir) )
  {
    r.err = "cannot create theme dir: " + dir;
    doki::msg_log("install '%s' failed: %s\n", def.displayName.c_str(), r.err.c_str());
    return r;
  }

  // Copy the sticker next to the css so the CSS $RELPATH can find it.
  CssOptions opt;
  if ( with_sticker && def.sticker.valid() )
  {
    const std::string src = path_join(path_join(assets_dir(), "stickers"), def.sticker.name);
    const std::string dst = path_join(dir, def.sticker.name);
    if ( copy_binary_file(src, dst) )
    {
      opt.include_sticker = true;
      opt.sticker_file = def.sticker.name;
      opt.sticker_anchor = def.sticker.anchor;
      r.sticker_installed = true;
    }
    else
    {
      doki::msg_log("  sticker not found, skipping: %s\n", src.c_str());
    }
  }

  // Optional wallpaper: prefer the explicit "background" block from the
  // definition; fall back to assets/wallpapers/<sticker name> for legacy
  // definitions that don't declare one.
  if ( with_wallpaper )
  {
    std::string wall_name;
    if ( def.background.valid() )
      wall_name = def.background.name;
    else if ( def.sticker.valid() )
      wall_name = def.sticker.name;

    if ( !wall_name.empty() )
    {
      const std::string src = path_join(path_join(assets_dir(), "wallpapers"), wall_name);
      const std::string dst = path_join(dir, WALLPAPER_FILE);
      if ( copy_binary_file(src, dst) )
      {
        opt.include_wallpaper = true;
        opt.wallpaper_file = WALLPAPER_FILE;
        opt.wallpaper_anchor = def.background.anchor;
        r.wallpaper_installed = true;
      }
      else
      {
        doki::msg_log("  wallpaper not found, skipping: %s\n", src.c_str());
      }
    }
  }

  const IdaPalette pal = map_theme(def);
  const std::string css = generate_theme_css(def, pal, opt);
  r.css_path = path_join(dir, "theme.css");
  if ( !write_text_file(r.css_path, css) )
  {
    r.err = "cannot write " + r.css_path;
    doki::msg_log("install '%s' failed: %s\n", def.displayName.c_str(), r.err.c_str());
    return r;
  }

  if ( activate )
    activate_theme(r.theme_name);

  r.ok = true;
  doki::msg_log("installed '%s' -> %s (sticker=%s, wallpaper=%s, active=%s)\n",
                def.displayName.c_str(), r.theme_name.c_str(),
                r.sticker_installed ? "yes" : "no",
                r.wallpaper_installed ? "yes" : "no",
                activate ? "yes" : "no");
  return r;
}

} // namespace doki
