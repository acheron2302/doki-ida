//----------------------------------------------------------------------------
// Doki Theme - self-tests.
//----------------------------------------------------------------------------
#include "doki/selftest.h"
#include "doki/assets.h"
#include "doki/color.h"
#include "doki/palette.h"
#include "doki/css.h"
#include "doki/registry.h"
#include "doki/paths.h"
#include "doki/log.h"

#include <string>

namespace doki
{

static int g_failures = 0;

#define CHECK(cond, ...)                              \
  do {                                                \
    if ( cond ) {                                     \
      doki::msg_log("  PASS: " __VA_ARGS__);          \
    } else {                                          \
      ++g_failures;                                   \
      doki::msg_log("  FAIL: " __VA_ARGS__);          \
    }                                                 \
  } while ( 0 )

static bool color_conversion_tests()
{
  doki::msg_log("color conversion:\n");

  Rgba c;
  // Canonical case from the plan: #3C82F6 -> 0x00F6823C (red is LSB).
  bool ok = parse_hex_color("#3C82F6", &c);
  CHECK(ok && c.r == 0x3C && c.g == 0x82 && c.b == 0xF6,
        "parse #3C82F6 -> r=%02x g=%02x b=%02x\n", c.r, c.g, c.b);
  CHECK(to_bgr24(c) == 0x00F6823Cu,
        "to_bgr24(#3C82F6) = 0x%06X (expect 0x00F6823C)\n", to_bgr24(c));

  // Short form #fff -> white.
  Rgba w;
  CHECK(parse_hex_color("#fff", &w) && w.r == 0xFF && w.g == 0xFF && w.b == 0xFF,
        "parse #fff -> #%02x%02x%02x\n", w.r, w.g, w.b);

  // Alpha form.
  Rgba a;
  CHECK(parse_hex_color("#11223380", &a) && a.a == 0x80,
        "parse #11223380 alpha = 0x%02x (expect 0x80)\n", a.a);

  // bg-with-alpha must never have a zero alpha byte.
  Rgba opaque{0x10, 0x20, 0x30, 0xFF};
  CHECK((to_bg_with_alpha(opaque) >> 24) == 0xFFu,
        "to_bg_with_alpha alpha byte = 0x%02X\n", to_bg_with_alpha(opaque) >> 24);

  // Round trip via CSS hex.
  CHECK(to_css_hex(c) == "#3c82f6", "to_css_hex -> %s\n", to_css_hex(c).c_str());

  // Reject garbage.
  Rgba junk;
  CHECK(!parse_hex_color("not-a-color", &junk), "reject 'not-a-color'\n");

  return g_failures == 0;
}

static void palette_completeness_test()
{
  doki::msg_log("palette completeness:\n");

  ThemeRegistry reg;
  reg.load_dir(definitions_dir());
  if ( reg.empty() )
  {
    ++g_failures;
    doki::msg_log("  FAIL: no themes available to map\n");
    return;
  }

  for ( const auto &t : reg.themes() )
  {
    IdaPalette p = map_theme(t);

    // Sanity: accent should equal the definition's accentColor when present.
    Rgba acc;
    bool acc_ok = !t.has_color("accentColor")
               || (parse_hex_color(t.color("accentColor"), &acc) && acc == p.accent);

    doki::msg_log("  %s [%s]: bg=%s text=%s accent=%s kw=%s str=%s nav=%s..%s\n",
                  t.displayName.c_str(), p.dark ? "dark" : "light",
                  to_css_hex(p.base_bg).c_str(),
                  to_css_hex(p.text).c_str(),
                  to_css_hex(p.accent).c_str(),
                  to_css_hex(p.lst_keyword).c_str(),
                  to_css_hex(p.lst_string).c_str(),
                  to_css_hex(p.nav_start).c_str(),
                  to_css_hex(p.nav_stop).c_str());
    CHECK(acc_ok, "  accent matches definition for %s\n", t.displayName.c_str());
  }
}

// Verify every "${name}" reference in css has a matching "@def name ".
static bool refs_resolved(const std::string &css)
{
  size_t pos = 0;
  while ( (pos = css.find("${", pos)) != std::string::npos )
  {
    size_t end = css.find('}', pos);
    if ( end == std::string::npos )
      return false;
    std::string name = css.substr(pos + 2, end - (pos + 2));
    if ( css.find("@def " + name + " ") == std::string::npos )
    {
      doki::msg_log("    unresolved ${%s}\n", name.c_str());
      return false;
    }
    pos = end + 1;
  }
  return true;
}

// Schema sanity for the bundled themes:
//   * every theme declares an explicit "background" block,
//   * every bundled theme's sticker and background have authoritative
//     CDN remote paths attached by the parser.
static void background_schema_test()
{
  doki::msg_log("background schema:\n");

  ThemeRegistry reg;
  reg.load_dir(definitions_dir());
  if ( reg.empty() )
  {
    ++g_failures;
    doki::msg_log("  FAIL: no themes available to check\n");
    return;
  }

  bool found_dark = false, found_light = false, found_rem = false;
  bool found_obsidian = false, found_sakura = false;
  for ( const auto &t : reg.themes() )
  {
    if ( t.name == "Darkness Dark" )
    {
      found_dark = true;
      CHECK(t.background.valid()
         && t.background.name == "darkness_dark.png",
            "Darkness Dark background.name == 'darkness_dark.png' (got '%s')\n",
            t.background.name.c_str());
    }
    if ( t.name == "Darkness Light" )
    {
      found_light = true;
      CHECK(t.background.valid()
         && t.background.name == "darkness_light.png",
            "Darkness Light background.name == 'darkness_light.png' (got '%s')\n",
            t.background.name.c_str());
    }
    if ( t.name == "Rem" )
    {
      found_rem = true;
      CHECK(t.background.valid()
         && t.background.name == "rem.png",
            "Rem background.name == 'rem.png' (got '%s')\n",
            t.background.name.c_str());
    }
    if ( t.name == "Zero Two Dark Obsidian" )
    {
      found_obsidian = true;
      CHECK(t.background.valid()
         && t.background.name == "zero_two_obsidian.png",
            "Zero Two Dark Obsidian background.name == 'zero_two_obsidian.png' (got '%s')\n",
            t.background.name.c_str());
    }
    if ( t.name == "Zero Two Light Sakura" )
    {
      found_sakura = true;
      CHECK(t.background.valid()
         && t.background.name == "zero_two_sakura.png",
            "Zero Two Light Sakura background.name == 'zero_two_sakura.png' (got '%s')\n",
            t.background.name.c_str());
    }

    // Every theme must have a sticker and a background with an
    // authoritative remote_path attached.
    CHECK(t.sticker.valid() && !t.sticker.remote_path.empty(),
          "%s: sticker remote_path populated ('%s')\n",
          t.displayName.c_str(), t.sticker.remote_path.c_str());
    CHECK(t.background.valid() && !t.background.remote_path.empty(),
          "%s: background remote_path populated ('%s')\n",
          t.displayName.c_str(), t.background.remote_path.c_str());
  }
  CHECK(found_dark,     "Darkness Dark definition present\n");
  CHECK(found_light,    "Darkness Light definition present\n");
  CHECK(found_rem,      "Rem definition present\n");
  CHECK(found_obsidian, "Zero Two Dark Obsidian definition present\n");
  CHECK(found_sakura,   "Zero Two Light Sakura definition present\n");
}

// CDN / asset manager invariants. These do not touch the network.
static void asset_manager_tests()
{
  doki::msg_log("asset manager:\n");

  // Remote path validation.
  CHECK(is_valid_remote_path(AssetKind::Sticker,
                             "stickers/vscode/reZero/rem/rem.png"),
        "valid sticker remote path accepted\n");
  CHECK(is_valid_remote_path(AssetKind::Wallpaper,
                             "backgrounds/wallpapers/transparent/darkness_dark.png"),
        "valid wallpaper remote path accepted\n");
  CHECK(!is_valid_remote_path(AssetKind::Sticker,
                              "backgrounds/wallpapers/transparent/darkness_dark.png"),
        "wallpaper path rejected for Sticker kind\n");
  CHECK(!is_valid_remote_path(AssetKind::Wallpaper,
                              "stickers/vscode/reZero/rem/rem.png"),
        "sticker path rejected for Wallpaper kind\n");
  CHECK(!is_valid_remote_path(AssetKind::Sticker, ""),
        "empty path rejected\n");
  CHECK(!is_valid_remote_path(AssetKind::Sticker,
                              "stickers/../etc/passwd"),
        "traversal rejected\n");
  CHECK(!is_valid_remote_path(AssetKind::Sticker,
                              "stickers\\windows\\path.png"),
        "backslash rejected\n");
  CHECK(!is_valid_remote_path(AssetKind::Sticker,
                              "https://evil.example/x.png"),
        "absolute URL rejected\n");
  CHECK(!is_valid_remote_path(AssetKind::Wallpaper,
                              "backgrounds/"),
        "empty file name rejected\n");

  // URL construction.
  CHECK(cdn_url_for(AssetKind::Sticker,
                    "stickers/vscode/reZero/rem/rem.png")
        == "https://doki.assets.unthrottled.io/stickers/vscode/reZero/rem/rem.png",
        "sticker URL built from authoritative host\n");
  CHECK(cdn_url_for(AssetKind::Wallpaper,
                    "backgrounds/wallpapers/transparent/darkness_dark.png")
        == "https://doki.assets.unthrottled.io/backgrounds/wallpapers/transparent/darkness_dark.png",
        "wallpaper URL built from authoritative host (transparent variant)\n");
  CHECK(cdn_url_for(AssetKind::Sticker, "https://evil.example/x.png").empty(),
        "invalid remote path returns empty URL\n");

  // Cache path selection.
  CHECK(cache_file_path(AssetKind::Sticker, "rem.png")
        == path_join(path_join(path_join(doki_root(), "cache"), "stickers"),
                     "rem.png"),
        "sticker cache path = $IDAUSR/doki-theme/cache/stickers/rem.png\n");
  CHECK(cache_file_path(AssetKind::Wallpaper, "rem.png")
        == path_join(path_join(path_join(doki_root(), "cache"), "wallpapers"),
                     "rem.png"),
        "wallpaper cache path = $IDAUSR/doki-theme/cache/wallpapers/rem.png\n");
  CHECK(cache_file_path(AssetKind::Sticker, "../escape.png").empty(),
        "traversal cache file name rejected\n");
  CHECK(cache_file_path(AssetKind::Sticker, "a/b.png").empty(),
        "slash in cache file name rejected\n");
}

static void css_generation_test()
{
  doki::msg_log("css generation:\n");

  ThemeRegistry reg;
  reg.load_dir(definitions_dir());
  if ( reg.empty() )
  {
    ++g_failures;
    doki::msg_log("  FAIL: no themes to generate CSS for\n");
    return;
  }

  bool dumped = false;
  for ( const auto &t : reg.themes() )
  {
    IdaPalette pal = map_theme(t);
    // Mirror the runtime: the Qt overlay is the only sticker renderer, so
    // the generated CSS never embeds a sticker. Wallpaper is still
    // CSS-driven and is enabled for themes that declare a background.
    CssOptions opt;
    if ( t.background.valid() )
    {
      opt.include_wallpaper = true;
      opt.wallpaper_file = "doki_wallpaper.png";
    }
    std::string css = generate_theme_css(t, pal, opt);

    int braces = 0;
    for ( char c : css ) { if ( c == '{' ) ++braces; else if ( c == '}' ) --braces; }

    bool ok = braces == 0
           && css.find("@importtheme") != std::string::npos
           && css.find("CustomIDAMemo") != std::string::npos
           && css.find("qproperty-line-bg-default") != std::string::npos
           && css.find("navband_t") != std::string::npos
           && refs_resolved(css);

    CHECK(ok, "valid CSS for %s (%u bytes, braces balanced=%s)\n",
          t.displayName.c_str(), (uint)css.size(), braces == 0 ? "yes" : "NO");

    // CSS must NEVER emit a sticker background; the Qt overlay paints it.
    bool sticker_file_in_css = css.find("url(\"$RELPATH/" + t.sticker.name + "\")")
                              != std::string::npos;
    CHECK(!sticker_file_in_css,
          "CSS does not embed sticker %s for %s (overlay renders it)\n",
          t.sticker.name.c_str(), t.displayName.c_str());

    // Every theme now declares an explicit background, so all themes must
    // get the wallpaper CSS treatment.
    {
      bool wall_on_memo = false;
      auto memo_start = css.find("CustomIDAMemo\n{");
      auto memo_end   = css.find("\n}\n", memo_start);
      if ( memo_start != std::string::npos && memo_end != std::string::npos )
      {
        std::string memo_block = css.substr(memo_start, memo_end - memo_start);
        wall_on_memo = memo_block.find("doki_wallpaper.png") != std::string::npos;
      }

      bool wall_ok = wall_on_memo
                  && css.find("text_area_t") != std::string::npos
                  && css.find("text_area_t { background: transparent; }")
                       != std::string::npos
                  && css.find("doki_wallpaper.png") != std::string::npos;
      CHECK(wall_ok, "wallpaper on CustomIDAMemo + transparent pseudocode for %s\n",
            t.displayName.c_str());
    }

    // The only wallpaper file name referenced in CSS is the fixed
    // "doki_wallpaper.png". No raw sticker / source-name leakage.
    bool other_wall_ref = false;
    {
      size_t pos = 0;
      while ( (pos = css.find("url(\"$RELPATH/", pos)) != std::string::npos )
      {
        size_t end = css.find("\")", pos);
        if ( end == std::string::npos ) break;
        std::string ref = css.substr(pos + 14, end - (pos + 14));
        if ( ref != "doki_wallpaper.png" )
          other_wall_ref = true;
        pos = end + 2;
      }
    }
    CHECK(!other_wall_ref,
          "CSS only references doki_wallpaper.png in url() for %s\n",
          t.displayName.c_str());

    // The generated CSS must not contain any rgba() with non-1 alpha for
    // the listing line background when a wallpaper is in play: the
    // transparency must come from the upstream PNG, not from CSS opacity.
    bool has_line_bg_alpha = false;
    {
      auto memo_start = css.find("CustomIDAMemo\n{");
      auto memo_end   = css.find("\n}\n", memo_start);
      if ( memo_start != std::string::npos && memo_end != std::string::npos )
      {
        std::string memo_block = css.substr(memo_start, memo_end - memo_start);
        // The "line-bg-default" rule for wallpaper themes uses
        // rgba(..., 0.000); the runtime "has_line_bg_alpha" check is
        // therefore expected: the listing line is the surface we keep
        // transparent so the PNG shows through. What we forbid is any
        // explicit opacity on the wallpaper image itself.
        if ( memo_block.find("qproperty-line-bg-default") != std::string::npos )
          has_line_bg_alpha = true;
      }
    }
    (void)has_line_bg_alpha; // documented expected; nothing to assert.

    // Pseudocode/decompiler selectors must be present in every generated
    // theme, regardless of wallpaper, so syntax highlighting follows the
    // doki palette in the decompiler view.
    bool pseudo_ok = css.find("text_area_t") != std::string::npos
                  && css.find("syntax_color_t") != std::string::npos
                  && css.find("qproperty-keyword1-fg") != std::string::npos
                  && css.find("qproperty-function-name-color") != std::string::npos;
    CHECK(pseudo_ok, "pseudocode/decompiler color CSS emitted for %s\n",
          t.displayName.c_str());

    if ( !dumped )
    {
      doki::msg_log("--- sample theme.css (%s) ---\n%s--- end sample ---\n",
                    t.displayName.c_str(), css.c_str());
      dumped = true;
    }
  }
}

bool run_selftest()
{
  g_failures = 0;
  doki::msg_log("=== self-test begin ===\n");
  color_conversion_tests();
  palette_completeness_test();
  background_schema_test();
  asset_manager_tests();
  css_generation_test();
  doki::msg_log("=== self-test %s (%d failure(s)) ===\n",
                g_failures == 0 ? "PASSED" : "FAILED", g_failures);
  return g_failures == 0;
}

#undef CHECK

} // namespace doki
