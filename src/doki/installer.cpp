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
#include "doki/fs.h"
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
    else if ( !prev_dash && !slug.empty() )
    {
      slug.push_back('-');
      prev_dash = true;
    }
  }
  while ( !slug.empty() && slug.back() == '-' )
    slug.pop_back();
  if ( slug.empty() )
    slug = "theme";
  return "doki-" + slug;
}

static bool write_text_file(const std::string &path, const std::string &content)
{
  std::ofstream ofs(doki::fs_path(path),
                    std::ios::binary | std::ios::trunc);
  if ( !ofs )
    return false;
  ofs.write(content.data(), (std::streamsize)content.size());
  ofs.flush();
  return (bool)ofs;
}

static bool copy_binary_file(const std::string &src, const std::string &dst)
{
  // IMPORTANT: do NOT consult (bool)in after the copy. With MSVC (and several
  // other stdlibs) reaching EOF sets both eofbit and failbit on the
  // ifstream, so a perfectly successful read would make the function
  // return false and the installer would silently skip the wallpaper
  // (leaving the cache file in place, the theme folder empty, and the
  // generated theme.css with no url(...) line). Only the writer's state
  // is reliable here.
  std::ifstream in(doki::fs_path(src), std::ios::binary);
  if ( !in )
    return false;
  std::ofstream out(doki::fs_path(dst),
                    std::ios::binary | std::ios::trunc);
  if ( !out )
    return false;
  out << in.rdbuf();
  out.flush();
  if ( in.bad() || !out )
    return false;
  return true;
}

// True for an existing, fully decodable PNG. Both stickers and wallpapers
// are PNGs; a valid header with a corrupt body must not be reused.
static bool is_usable_png(const std::string &path)
{
  return is_decodable_png_file(path);
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
// allow_download=false only consults the existing cache (used during plugin
// init migration so startup never blocks on the network).
static std::string resolve_sticker_source(const DokiSticker &s,
                                          bool allow_download)
{
  if ( s.name.empty() )
    return std::string();
  // 1) Authoritative CDN cache (downloaded on demand).
  if ( !s.remote_path.empty() )
  {
    const std::string cached = allow_download
        ? ensure_asset(AssetKind::Sticker, s.name, s.remote_path)
        : cache_file_path(AssetKind::Sticker, s.name);
    if ( !cached.empty() && is_usable_png(cached) )
      return cached;
  }
  // Cache may exist even when remotePath is absent (custom color-only
  // overrides) or when the download was coalesced/failed.
  const std::string cached_path = cache_file_path(AssetKind::Sticker, s.name);
  if ( is_usable_png(cached_path) )
    return cached_path;
  // 2) Legacy / offline fallback: $IDAUSR/doki-theme/assets/stickers/<name>.
  const std::string legacy = path_join(
      path_join(assets_dir(), "stickers"), s.name);
  if ( is_usable_png(legacy) )
    return legacy;
  return std::string();
}

static std::string resolve_wallpaper_source(const DokiBackground &b,
                                            bool allow_download)
{
  if ( b.name.empty() )
    return std::string();
  // 1) Authoritative CDN cache.
  if ( !b.remote_path.empty() )
  {
    const std::string cached = allow_download
        ? ensure_asset(AssetKind::Wallpaper, b.name, b.remote_path)
        : cache_file_path(AssetKind::Wallpaper, b.name);
    if ( !cached.empty() && is_usable_png(cached) )
      return cached;
  }
  // Cache may exist even when remotePath is absent or the fetch was skipped.
  const std::string cached_path = cache_file_path(AssetKind::Wallpaper, b.name);
  if ( is_usable_png(cached_path) )
    return cached_path;
  // 2) Legacy / offline fallback.
  const std::string legacy = path_join(
      path_join(assets_dir(), "wallpapers"), b.name);
  if ( is_usable_png(legacy) )
    return legacy;
  return std::string();
}

InstallResult install_theme(const DokiThemeDefinition &def, bool activate,
                            bool with_sticker, bool with_wallpaper,
                            bool allow_download)
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
    if ( !is_safe_leaf_name(def.sticker.name) )
    {
      doki::msg_log("  sticker name is not a safe file name, skipping: %s\n",
                    def.sticker.name.c_str());
    }
    else
    {
      const std::string dst = path_join(dir, def.sticker.name);
      if ( is_usable_png(dst) )
      {
        // A previous apply already installed the sticker. Reuse it without
        // touching the network/cache (also keeps offline upgrades fast).
        doki::msg_log("  sticker using previously installed copy: %s\n",
                      def.sticker.name.c_str());
        r.sticker_installed = true;
      }
      else
      {
        const std::string src = resolve_sticker_source(def.sticker, allow_download);
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
    }
  }

  // Optional wallpaper. Every bundled theme declares a "background" block
  // (the parser fills its remote_path from the authoritative CDN lookup).
  if ( with_wallpaper && def.background.valid() )
  {
    const std::string dst = path_join(dir, WALLPAPER_FILE);
    bool available = is_usable_png(dst);
    if ( available )
    {
      doki::msg_log("  wallpaper using previously installed copy: %s\n",
                    WALLPAPER_FILE);
    }
    else
    {
      const std::string src = resolve_wallpaper_source(def.background, allow_download);
      available = !src.empty() && copy_binary_file(src, dst);
    }
    if ( available )
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
  // Reflect the actual opt (which gates the CSS url() line) so the log
  // can't disagree with the generated theme.css. r.wallpaper_installed
  // and opt.include_wallpaper are set in the same block, so they're
  // always equal in practice — but logging the opt makes any future
  // divergence visible immediately.
  doki::msg_log("installed '%s' -> %s (sticker=%s, wallpaper=%s, active=%s)\n",
                def.displayName.c_str(), r.theme_name.c_str(),
                r.sticker_installed ? "yes" : "no",
                opt.include_wallpaper ? "yes" : "no",
                activate ? "yes" : "no");
  return r;
}

} // namespace doki
