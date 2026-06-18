//----------------------------------------------------------------------------
// Doki Theme - installer implementation.
//
// Installs a definition as a real IDA theme folder under
// $IDAUSR/themes/<theme_name>/. The folder contains a generated theme.css
// plus the sticker image and the wallpaper (when present) so that the
// CSS $RELPATH lookup resolves. Activation sets HKCU\..\IDA\ThemeName,
// which IDA reads to pick the theme at startup.
//
// Sticker and wallpaper assets are normally obtained on demand from the
// authoritative Doki CDN via the asset manager. The legacy
// $IDAUSR/doki-theme/assets/{stickers,wallpapers}/ folders are accepted as
// a compatibility / offline fallback only.
//----------------------------------------------------------------------------
#include "doki/installer.h"
#include "doki/assets.h"
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

// Resolve a local source for an asset. The CDN cache is the primary path;
// the legacy assets/ folder is accepted as a compatibility fallback.
static std::string resolve_sticker_source(const DokiSticker &s)
{
  if ( s.name.empty() )
    return std::string();
  // 1) Authoritative CDN cache (downloaded on demand).
  if ( !s.remote_path.empty() )
  {
    const std::string cached = ensure_asset(AssetKind::Sticker, s.name,
                                            s.remote_path);
    if ( !cached.empty() )
      return cached;
  }
  // 2) Legacy / offline fallback: $IDAUSR/doki-theme/assets/stickers/<name>.
  const std::string legacy = path_join(
      path_join(assets_dir(), "stickers"), s.name);
  std::ifstream test(legacy, std::ios::binary);
  if ( test )
    return legacy;
  return std::string();
}

static std::string resolve_wallpaper_source(const DokiBackground &b)
{
  if ( b.name.empty() )
    return std::string();
  // 1) Authoritative CDN cache.
  if ( !b.remote_path.empty() )
  {
    const std::string cached = ensure_asset(AssetKind::Wallpaper, b.name,
                                            b.remote_path);
    if ( !cached.empty() )
      return cached;
  }
  // 2) Legacy / offline fallback.
  const std::string legacy = path_join(
      path_join(assets_dir(), "wallpapers"), b.name);
  std::ifstream test(legacy, std::ios::binary);
  if ( test )
    return legacy;
  return std::string();
}

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

  // Copy the sticker next to the css so the overlay manager can find it
  // at runtime. The Qt overlay (DokiOverlayManager) is the sole sticker
  // renderer; we explicitly do NOT set opt.include_sticker (removed) so
  // the generated CSS never paints a duplicate sticker on CustomIDAMemo.
  CssOptions opt;
  if ( with_sticker && def.sticker.valid() )
  {
    const std::string src = resolve_sticker_source(def.sticker);
    const std::string dst = path_join(dir, def.sticker.name);
    if ( !src.empty() && copy_binary_file(src, dst) )
    {
      // Sticker is rendered by the Qt overlay; CSS keeps it out.
      r.sticker_installed = true;
    }
    else
    {
      doki::msg_log("  sticker not available, skipping: %s\n",
                    def.sticker.name.c_str());
    }
  }

  // Optional wallpaper. Every bundled theme declares a "background" block
  // (the parser fills its remote_path from the authoritative CDN lookup).
  if ( with_wallpaper && def.background.valid() )
  {
    const std::string src = resolve_wallpaper_source(def.background);
    const std::string dst = path_join(dir, WALLPAPER_FILE);
    if ( !src.empty() && copy_binary_file(src, dst) )
    {
      opt.include_wallpaper = true;
      opt.wallpaper_file = WALLPAPER_FILE;
      opt.wallpaper_anchor = def.background.anchor;
      r.wallpaper_installed = true;
      r.wallpaper_file = WALLPAPER_FILE;
    }
    else
    {
      doki::msg_log("  wallpaper not available, skipping: %s\n",
                    def.background.name.c_str());
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
