//----------------------------------------------------------------------------
// Doki Theme - self-tests.
//----------------------------------------------------------------------------
#include "doki/selftest.h"
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
    CssOptions opt;
    opt.include_sticker = true;
    opt.sticker_file = t.sticker.name;
    opt.sticker_anchor = t.sticker.anchor;
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
  css_generation_test();
  doki::msg_log("=== self-test %s (%d failure(s)) ===\n",
                g_failures == 0 ? "PASSED" : "FAILED", g_failures);
  return g_failures == 0;
}

#undef CHECK

} // namespace doki
