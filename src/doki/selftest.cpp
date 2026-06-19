//----------------------------------------------------------------------------
// Doki Theme - self-tests.
//
// Headless: runs in non-GUI mode against the deployed catalog
// (generated/theme_catalog.json -> $IDAUSR/doki-theme/theme_catalog.json)
// when available, and otherwise against the bundled definitions/
// directory. The tests assert catalog-level invariants rather than
// hard-coding specific theme names.
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
#include <set>

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

static void load_registry_with_fallback(ThemeRegistry &reg)
{
  // Prefer the deployed catalog; fall back to the legacy
  // definitions/ directory when the catalog is missing (e.g. local
  // development without an explicit deploy).
  if ( reg.load_catalog(catalog_path()) == 0 )
  {
    doki::msg_log("  catalog not deployed, falling back to %s\n",
                  definitions_dir().c_str());
    reg.load_dir(definitions_dir());
  }
}

static void palette_completeness_test()
{
  doki::msg_log("palette completeness:\n");

  ThemeRegistry reg;
  load_registry_with_fallback(reg);
  if ( reg.empty() )
  {
    ++g_failures;
    doki::msg_log("  FAIL: no themes available to map\n");
    return;
  }

  for ( const auto &t : reg.themes() )
  {
    IdaPalette p = map_theme(t);

    // Phase 1: ui_accent / editor_accent / accent split.
    //   - p.accent must always equal p.ui_accent.
    //   - If the definition exposes chrome_accent_color (pre-merge base
    //     accent captured by theme.cpp), p.ui_accent must equal that.
    //   - Else p.ui_accent must equal the merged accentColor.
    //   - If the definition exposes editor_accent_color, p.editor_accent
    //     must equal that; else it must fall back to p.ui_accent.
    CHECK(p.accent == p.ui_accent,
          "  accent aliases ui_accent for %s\n", t.displayName.c_str());

    doki::msg_log("  %s [%s]: bg=%s text=%s accent=%s kw=%s str=%s nav=%s..%s\n",
                  t.displayName.c_str(), p.dark ? "dark" : "light",
                  to_css_hex(p.base_bg).c_str(),
                  to_css_hex(p.text).c_str(),
                  to_css_hex(p.accent).c_str(),
                  to_css_hex(p.lst_keyword).c_str(),
                  to_css_hex(p.lst_string).c_str(),
                  to_css_hex(p.nav_start).c_str(),
                  to_css_hex(p.nav_stop).c_str());

    if ( !t.chrome_accent_color.empty() )
    {
      Rgba cacc;
      bool cacc_ok = parse_hex_color(t.chrome_accent_color, &cacc)
                  && cacc == p.ui_accent;
      CHECK(cacc_ok,
            "  ui_accent=%s matches captured chrome_accent for %s (expected %s)\n",
            to_css_hex(p.ui_accent).c_str(),
            t.displayName.c_str(),
            t.chrome_accent_color.c_str());
    }
    else if ( t.has_color("accentColor") )
    {
      Rgba acc;
      bool acc_ok = parse_hex_color(t.color("accentColor"), &acc)
                 && acc == p.ui_accent;
      CHECK(acc_ok,
            "  ui_accent=%s matches merged accentColor for %s (expected %s)\n",
            to_css_hex(p.ui_accent).c_str(),
            t.displayName.c_str(),
            t.color("accentColor").c_str());
    }

    if ( !t.editor_accent_color.empty() )
    {
      Rgba eacc;
      bool eacc_ok = parse_hex_color(t.editor_accent_color, &eacc)
                  && eacc == p.editor_accent;
      CHECK(eacc_ok,
            "  editor_accent=%s matches captured override for %s (expected %s)\n",
            to_css_hex(p.editor_accent).c_str(),
            t.displayName.c_str(),
            t.editor_accent_color.c_str());
    }
    else
    {
      CHECK(p.editor_accent == p.ui_accent,
            "  editor_accent falls back to ui_accent when no override (%s)\n",
            t.displayName.c_str());
    }
  }
}

// Look up a definition's catalog value and parse it. Logs and returns
// false if the key is missing or malformed, so callers can skip the
// comparison instead of using an uninitialized Rgba.
static bool parse_expected_color(const DokiThemeDefinition &def,
                                 const char *key,
                                 Rgba *out)
{
  if ( !def.has_color(key) )
  {
    doki::msg_log("    missing expected color %s\n", key);
    return false;
  }
  if ( !parse_hex_color(def.color(key), out) )
  {
    doki::msg_log("    invalid expected color %s='%s'\n",
                  key, def.color(key).c_str());
    return false;
  }
  return true;
}

// Spot-check Echidna: confirms the editor-accent split, selection alpha,
// and the new chrome/breakpoint roles map to the expected catalog values.
static void echidna_specific_test()
{
  doki::msg_log("echidna invariants:\n");

  ThemeRegistry reg;
  if ( reg.load_catalog(catalog_path()) == 0 )
  {
    doki::msg_log("  (catalog not deployed; skipping Echidna spot-check)\n");
    return;
  }

  const DokiThemeDefinition *echidna = nullptr;
  for ( const auto &t : reg.themes() )
  {
    if ( t.name == "Echidna" ) { echidna = &t; break; }
  }
  if ( echidna == nullptr )
  {
    ++g_failures;
    doki::msg_log("  FAIL: Echidna definition not found in catalog\n");
    return;
  }

  IdaPalette p = map_theme(*echidna);

  // Phase 1 split: chrome accent is the base near-white #ececec, while
  // the listing/decompiler accent is the editorScheme override #7ceec8.
  Rgba mint;       parse_hex_color("#7ceec8", &mint);
  Rgba near_white; parse_hex_color("#ececec", &near_white);
  CHECK(p.ui_accent == near_white,
        "  Echidna ui_accent=%s (expected #ececec)\n",
        to_css_hex(p.ui_accent).c_str());
  CHECK(p.accent == p.ui_accent,
        "  Echidna accent aliases ui_accent\n");
  CHECK(p.editor_accent == mint,
        "  Echidna editor_accent=%s (expected #7ceec8)\n",
        to_css_hex(p.editor_accent).c_str());
  CHECK(echidna->chrome_accent_color == "#ececec",
        "  Echidna chrome_accent_color='%s' (expected #ececec)\n",
        echidna->chrome_accent_color.c_str());
  CHECK(echidna->editor_accent_color == "#7ceec8",
        "  Echidna editor_accent_color='%s' (expected #7ceec8)\n",
        echidna->editor_accent_color.c_str());

  // selectionBackgroundTransparent = #274540aa -> alpha byte 0xAA survives.
  Rgba sel_alpha; parse_hex_color("#274540aa", &sel_alpha);
  CHECK(p.selection_bg_alpha == sel_alpha && p.selection_bg_alpha.a == 0xAA,
        "  Echidna selection_bg_alpha=%s a=%u (expected #274540aa a=170)\n",
        to_css_rgba(p.selection_bg_alpha).c_str(),
        (uint)p.selection_bg_alpha.a);

  // Chrome roles pulled from catalog values.
  Rgba hdr;
  CHECK(parse_expected_color(*echidna, "headerColor", &hdr)
        && p.chrome_header == hdr,
        "  Echidna chrome_header=%s (expected %s)\n",
        to_css_hex(p.chrome_header).c_str(),
        echidna->color("headerColor").c_str());
  Rgba contrast;
  CHECK(parse_expected_color(*echidna, "contrastColor", &contrast)
        && p.chrome_contrast == contrast,
        "  Echidna chrome_contrast=%s (expected %s)\n",
        to_css_hex(p.chrome_contrast).c_str(),
        echidna->color("contrastColor").c_str());

  // Breakpoint colors.
  Rgba bpa;
  CHECK(parse_expected_color(*echidna, "breakpointActiveColor", &bpa)
        && p.bp_active == bpa,
        "  Echidna bp_active=%s (expected %s)\n",
        to_css_hex(p.bp_active).c_str(),
        echidna->color("breakpointActiveColor").c_str());
  Rgba bpi;
  CHECK(parse_expected_color(*echidna, "breakpointColor", &bpi)
        && p.bp_inactive == bpi,
        "  Echidna bp_inactive=%s (expected %s)\n",
        to_css_hex(p.bp_inactive).c_str(),
        echidna->color("breakpointColor").c_str());

  // Search roles.
  Rgba sbg;
  CHECK(parse_expected_color(*echidna, "searchBackground", &sbg)
        && p.search_bg == sbg,
        "  Echidna search_bg=%s (expected %s)\n",
        to_css_hex(p.search_bg).c_str(),
        echidna->color("searchBackground").c_str());

  // Accent transparency utilities.
  Rgba a35;
  CHECK(parse_expected_color(*echidna, "accentColorTransparent", &a35)
        && p.accent_a35 == a35 && p.accent_a35.a == 0x5A,
        "  Echidna accent_a35=%s a=%u (expected alpha=0x5A)\n",
        to_css_rgba(p.accent_a35).c_str(),
        (uint)p.accent_a35.a);

  // lst_info / lst_unused pull from infoForeground / unusedColor.
  Rgba info;
  CHECK(parse_expected_color(*echidna, "infoForeground", &info)
        && p.lst_info == info,
        "  Echidna lst_info=%s (expected %s)\n",
        to_css_hex(p.lst_info).c_str(),
        echidna->color("infoForeground").c_str());
  Rgba unused;
  CHECK(parse_expected_color(*echidna, "unusedColor", &unused)
        && p.lst_unused == unused,
        "  Echidna lst_unused=%s (expected %s)\n",
        to_css_hex(p.lst_unused).c_str(),
        echidna->color("unusedColor").c_str());
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

// Reverse check: every emitted "@def name" must be referenced by at least
// one "${name}" consumer in the same CSS body. Catches dead variables that
// refs_resolved() cannot see (refs only looks at the forward direction).
static bool defs_used(const std::string &css)
{
  size_t pos = 0;
  bool ok = true;
  while ( (pos = css.find("@def ", pos)) != std::string::npos )
  {
    size_t name_start = pos + 5;
    size_t name_end = css.find(' ', name_start);
    if ( name_end == std::string::npos ) return false;
    std::string name = css.substr(name_start, name_end - name_start);
    if ( css.find("${" + name + "}") == std::string::npos )
    {
      doki::msg_log("    unused @def %s\n", name.c_str());
      ok = false;
    }
    pos = name_end;
  }
  return ok;
}

// Catalog-level invariants. These are the tests the generator and
// runtime are expected to satisfy together. They do not depend on
// any specific theme name.
static void catalog_invariants_test()
{
  doki::msg_log("catalog invariants:\n");

  ThemeRegistry reg;
  size_t loaded = reg.load_catalog(catalog_path());
  if ( loaded == 0 )
  {
    doki::msg_log("  (no catalog deployed; skipping catalog-level invariants)\n");
    return;
  }

  // 1. The catalog must have a healthy theme count.
  CHECK(loaded >= 10,
        "catalog has a reasonable theme count (%u >= 10)\n", (uint)loaded);

  std::set<std::string> seen_ids;
  std::set<std::string> seen_sticker_paths;
  std::set<std::string> seen_wallpaper_paths;

  for ( const auto &t : reg.themes() )
  {
    // 2. Every theme has its core fields.
    CHECK(!t.id.empty(),                "%s: id is set\n", t.displayName.c_str());
    CHECK(!t.name.empty(),              "%s: name is set\n", t.displayName.c_str());
    CHECK(!t.displayName.empty(),       "%s: displayName is set\n", t.displayName.c_str());
    CHECK(t.has_color("baseBackground"), "%s: baseBackground present\n", t.displayName.c_str());

    // 3. IDs are unique.
    bool id_new = seen_ids.insert(t.id).second;
    CHECK(id_new, "id '%s' (%s) is unique\n",
          t.id.c_str(), t.displayName.c_str());

    // 4. Sticker remote path is well-formed and unique-per-theme.
    CHECK(t.sticker.valid(),            "%s: sticker declared\n", t.displayName.c_str());
    CHECK(!t.sticker.remote_path.empty(),
          "%s: sticker remote_path populated ('%s')\n",
          t.displayName.c_str(), t.sticker.remote_path.c_str());
    CHECK(t.sticker.remote_path.rfind("stickers/vscode/", 0) == 0,
          "%s: sticker remote_path starts with stickers/vscode/ ('%s')\n",
          t.displayName.c_str(), t.sticker.remote_path.c_str());
    bool stk_new = seen_sticker_paths.insert(t.sticker.remote_path).second;
    CHECK(stk_new, "sticker remote_path '%s' is unique\n",
          t.sticker.remote_path.c_str());

    // 5. Wallpaper remote path uses the transparent variant.
    CHECK(t.background.valid(),         "%s: background declared\n", t.displayName.c_str());
    CHECK(!t.background.remote_path.empty(),
          "%s: background remote_path populated ('%s')\n",
          t.displayName.c_str(), t.background.remote_path.c_str());
    CHECK(t.background.remote_path.rfind("backgrounds/wallpapers/transparent/", 0) == 0,
          "%s: background remote_path uses transparent variant ('%s')\n",
          t.displayName.c_str(), t.background.remote_path.c_str());
    bool wall_new = seen_wallpaper_paths.insert(t.background.remote_path).second;
    CHECK(wall_new, "background remote_path '%s' is unique\n",
          t.background.remote_path.c_str());

    // 6. Reject traversal / absolute URLs in remote paths.
    auto check_safe = [](const std::string &p) -> bool
    {
      return p.find("..") == std::string::npos
          && p.find('\\') == std::string::npos
          && p.find("://") == std::string::npos;
    };
    CHECK(check_safe(t.sticker.remote_path),
          "%s: sticker remote_path has no traversal/backslash/absolute URL\n",
          t.displayName.c_str());
    CHECK(check_safe(t.background.remote_path),
          "%s: background remote_path has no traversal/backslash/absolute URL\n",
          t.displayName.c_str());
  }
}

// Spot-check the four well-known themes from the plan. These are
// upstream names that should always be present in the catalog.
static void known_themes_test()
{
  doki::msg_log("known themes:\n");

  ThemeRegistry reg;
  if ( reg.load_catalog(catalog_path()) == 0 ) return;

  struct Probe
  {
    const char *name;
    const char *sticker_path;
    const char *wallpaper_path;
  };
  Probe probes[] = {
    { "Rem",
      "stickers/vscode/reZero/rem/rem.png",
      "backgrounds/wallpapers/transparent/rem.png" },
    { "Zero Two Dark Obsidian",
      "stickers/vscode/franxx/zeroTwo/obsidian/zero_two_obsidian.png",
      "backgrounds/wallpapers/transparent/zero_two_obsidian.png" },
    { "Zero Two Light Sakura",
      "stickers/vscode/franxx/zeroTwo/sakura/zero_two_sakura.png",
      "backgrounds/wallpapers/transparent/zero_two_sakura.png" },
    { "Darkness Dark",
      "stickers/vscode/konoSuba/darkness/dark/darkness_dark.png",
      "backgrounds/wallpapers/transparent/darkness_dark.png" },
    { "Darkness Light",
      "stickers/vscode/konoSuba/darkness/light/darkness_light.png",
      "backgrounds/wallpapers/transparent/darkness_light.png" },
  };

  for ( const auto &p : probes )
  {
    bool found = false;
    for ( const auto &t : reg.themes() )
    {
      if ( t.name == p.name )
      {
        found = true;
        CHECK(t.sticker.remote_path == p.sticker_path,
              "%s: sticker='%s' (expect '%s')\n",
              p.name, t.sticker.remote_path.c_str(), p.sticker_path);
        CHECK(t.background.remote_path == p.wallpaper_path,
              "%s: wallpaper='%s' (expect '%s')\n",
              p.name, t.background.remote_path.c_str(), p.wallpaper_path);
        break;
      }
    }
    CHECK(found, "%s: definition present in catalog\n", p.name);
  }
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

// ThemeRegistry::upsert: same id replaces in-place; new id appends.
static void registry_upsert_test()
{
  doki::msg_log("registry upsert:\n");

  ThemeRegistry reg;

  DokiThemeDefinition a;
  a.id = "id-a";
  a.name = "OriginalA";
  a.displayName = "Original A";
  a.group = "Test";
  a.colors["baseBackground"] = "#101010";
  CHECK(!reg.upsert(a), "first upsert of id-a appends (returns false)\n");
  CHECK(reg.size() == 1, "size=1 after first append\n");
  CHECK(reg.get("id-a") != nullptr, "id-a present after append\n");
  CHECK(reg.get("id-a")->name == "OriginalA",
        "id-a name is the original\n");

  // Replace in place: same id, different name.
  DokiThemeDefinition a2;
  a2.id = "id-a";
  a2.name = "OverrideA";
  a2.displayName = "Override A";
  a2.group = "Test";
  a2.colors["baseBackground"] = "#202020";
  CHECK(reg.upsert(a2),
        "second upsert of same id-a replaces (returns true)\n");
  CHECK(reg.size() == 1,
        "size still 1 after replacement (no duplicate created)\n");
  CHECK(reg.get("id-a") != nullptr, "id-a still present after replace\n");
  CHECK(reg.get("id-a")->name == "OverrideA",
        "id-a name updated to OverrideA\n");
  CHECK(reg.get("id-a")->colors.at("baseBackground") == "#202020",
        "id-a colors replaced (baseBackground is now #202020)\n");

  // New id: append.
  DokiThemeDefinition b;
  b.id = "id-b";
  b.name = "BrandNewB";
  b.displayName = "Brand New B";
  b.group = "Test";
  b.colors["baseBackground"] = "#303030";
  CHECK(!reg.upsert(b), "upsert of new id-b appends (returns false)\n");
  CHECK(reg.size() == 2, "size=2 after second distinct id\n");
  CHECK(reg.get("id-b") != nullptr, "id-b present after append\n");

  // Same-id replacement must NOT create a hidden duplicate: scanning
  // the underlying vector for duplicates catches a regression where
  // upsert fell back to push_back instead of in-place replace.
  size_t count_a = 0;
  for ( const auto &t : reg.themes() )
    if ( t.id == "id-a" )
      ++count_a;
  CHECK(count_a == 1,
        "exactly one id-a entry exists after replacement (no duplicates)\n");
}

static void css_generation_test()
{
  doki::msg_log("css generation:\n");

  ThemeRegistry reg;
  load_registry_with_fallback(reg);
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
           && refs_resolved(css)
           && defs_used(css);

    // Phase 2-8: every new widget block must be emitted.
    bool phase28 = css.find("qproperty-line-fg-import-name") != std::string::npos
                && css.find("qproperty-line-fg-unknown-name") != std::string::npos
                && css.find("qproperty-line-fg-code-xref") != std::string::npos
                && css.find("qproperty-line-fg-opcode-byte") != std::string::npos
                && css.find("qproperty-line-bgovl-current-line") != std::string::npos
                && css.find("qproperty-line-pfx-current-item") != std::string::npos
                && css.find("qproperty-line-bg-bpt-enabled") != std::string::npos
                && css.find("qproperty-graph-bg-top") != std::string::npos
                && css.find("qproperty-graph-edge-yes") != std::string::npos
                && css.find("qproperty-line-fg-deref-code") != std::string::npos
                && css.find("TextArrows") != std::string::npos
                && css.find("TCpuRegs") != std::string::npos;

    // Phase 9: Qt chrome overrides.
    bool phase9 = css.find("QToolBar") != std::string::npos
               && css.find("QMenuBar") != std::string::npos
               && css.find("QGroupBox") != std::string::npos
               && css.find("QToolTip") != std::string::npos;

    // Phase 1 split: the editor-accent var exists and at least one
    // qproperty consumes it (so refs_resolved above also catches it, but a
    // positive check on a real consumer is more diagnostic on failure).
    bool phase1 = css.find("@def doki_eaccent") != std::string::npos
               && css.find("qproperty-line-fg-code-xref          : ${doki_eaccent};")
                      != std::string::npos
               && css.find("qproperty-code-reference-color       : ${doki_eaccent};")
                      != std::string::npos;

    CHECK(phase28, "Phase 2-8 selectors emitted for %s\n", t.displayName.c_str());
    CHECK(phase9,  "Phase 9 Qt chrome emitted for %s\n",   t.displayName.c_str());
    CHECK(phase1,  "Phase 1 accent split vars emitted for %s\n",
          t.displayName.c_str());

    CHECK(ok, "valid CSS for %s (%u bytes, braces balanced=%s)\n",
          t.displayName.c_str(), (uint)css.size(), braces == 0 ? "yes" : "NO");

    // CSS must NEVER emit a sticker background; the Qt overlay paints it.
    if ( t.sticker.valid() )
    {
      bool sticker_file_in_css =
          css.find("url(\"$RELPATH/" + t.sticker.name + "\")")
          != std::string::npos;
      CHECK(!sticker_file_in_css,
            "CSS does not embed sticker %s for %s (overlay renders it)\n",
            t.sticker.name.c_str(), t.displayName.c_str());
    }

    // Themes that declare a background get the wallpaper CSS treatment.
    if ( t.background.valid() )
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
  (void)color_conversion_tests();
  catalog_invariants_test();
  known_themes_test();
  palette_completeness_test();
  echidna_specific_test();
  asset_manager_tests();
  registry_upsert_test();
  css_generation_test();
  doki::msg_log("=== self-test %s (%d failure(s)) ===\n",
                g_failures == 0 ? "PASSED" : "FAILED", g_failures);
  return g_failures == 0;
}

#undef CHECK

} // namespace doki
