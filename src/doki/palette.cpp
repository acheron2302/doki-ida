//----------------------------------------------------------------------------
// Doki Theme - palette mapping implementation.
//----------------------------------------------------------------------------
#include "doki/palette.h"

#include <initializer_list>

namespace doki
{

// Return the first key in 'keys' that exists and parses; else 'fallback'.
static Rgba pick(const DokiThemeDefinition &def,
                 std::initializer_list<const char *> keys,
                 const Rgba &fallback)
{
  for ( const char *k : keys )
  {
    if ( def.has_color(k) )
    {
      Rgba c;
      if ( parse_hex_color(def.color(k), &c) )
        return c;
    }
  }
  return fallback;
}

IdaPalette map_theme(const DokiThemeDefinition &def)
{
  IdaPalette p;
  p.dark = def.dark;

  // Theme-wide defaults used only when a definition omits a key entirely.
  const Rgba def_bg   = def.dark ? Rgba{0x1e,0x1e,0x1e,255} : Rgba{0xf5,0xf5,0xf5,255};
  const Rgba def_fg   = def.dark ? Rgba{0xbb,0xbb,0xbb,255} : Rgba{0x25,0x25,0x25,255};
  const Rgba def_acc  = def.dark ? Rgba{0xff,0x6a,0xc1,255} : Rgba{0xa6,0x22,0x10,255};
  const Rgba def_mut  = def.dark ? Rgba{0x80,0x80,0x80,255} : Rgba{0x99,0x99,0x99,255};

  // --- core anchors ---
  p.window_bg    = pick(def, {"baseBackground"}, def_bg);
  p.base_bg      = pick(def, {"textEditorBackground", "baseBackground"}, p.window_bg);
  p.text         = pick(def, {"foregroundColor"}, def_fg);
  p.accent       = pick(def, {"accentColor"}, def_acc);

  // --- UI chrome ---
  p.secondary_bg = pick(def, {"secondaryBackground", "headerColor", "baseBackground"}, p.window_bg);
  p.selection_bg = pick(def, {"selectionBackground"}, p.accent);
  p.selection_fg = pick(def, {"selectionForeground"}, p.text);
  p.button_bg    = pick(def, {"buttonColor", "secondaryBackground"}, p.secondary_bg);
  p.button_fg    = pick(def, {"buttonFont", "foregroundColor"}, p.text);
  p.border       = pick(def, {"borderColor", "secondaryBackground"}, p.secondary_bg);
  p.line_number  = pick(def, {"lineNumberColor", "comments"}, def_mut);
  p.caret_row    = pick(def, {"caretRow", "highlightColor"}, p.secondary_bg);
  p.disabled     = pick(def, {"disabledColor", "comments"}, def_mut);

  // --- listing / syntax ---
  p.lst_bg       = p.base_bg;
  p.lst_text     = pick(def, {"foregroundColor"}, def_fg);
  p.lst_comment  = pick(def, {"comments"}, def_mut);
  p.lst_keyword  = pick(def, {"keywordColor", "accentColor"}, p.accent);
  p.lst_string   = pick(def, {"stringColor"}, p.accent);
  p.lst_number   = pick(def, {"constantColor", "accentColor"}, p.accent);
  p.lst_name     = pick(def, {"classNameColor", "foregroundColor"}, p.text);
  p.lst_register = pick(def, {"keyColor", "accentColor"}, p.accent);

  // --- nav band ---
  // The nav band should look like IDA's default band, not the doki accent.
  // We fall back to a neutral dark/light gray (matching IDA's built-in nav
  // band tint) instead of `p.accent`. Themes that explicitly define
  // startColor / stopColor / highlightColor still get a gradient.
  const Rgba def_nav = def.dark
                      ? Rgba{0x1f,0x1f,0x1f,255}
                      : Rgba{0xf0,0xf0,0xf0,255};
  const Rgba def_nav_hi = def.dark
                         ? Rgba{0x39,0x5a,0x94,255}
                         : Rgba{0xc4,0xd2,0xea,255};
  p.nav_start     = pick(def, {"startColor"}, def_nav);
  p.nav_stop      = pick(def, {"stopColor"},  def_nav);
  p.nav_highlight = pick(def, {"highlightColor"}, def_nav_hi);

  return p;
}

} // namespace doki
