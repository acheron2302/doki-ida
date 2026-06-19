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

// Parse a single raw color string. Returns fallback on absence or parse
// failure. Used for the raw accent snapshots captured in theme.cpp before
// any merge, which are not routed through pick().
static Rgba parse_or(const std::string &value, const Rgba &fallback)
{
  Rgba c;
  return parse_hex_color(value, &c) ? c : fallback;
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

  // Phase 1: editor_accent / ui_accent split.
  //
  // chrome_accent_color (captured in theme.cpp BEFORE the editorScheme
  // override merge overwrites def.colors["accentColor"]) is the upstream
  // chrome accent. For Echidna it is the base near-white #ececec; the
  // editorScheme override (#7ceec8 mint) only affects listing/decompiler
  // surfaces and is captured separately as editor_accent_color.
  //
  // If the definition has no base accent, ui_accent falls back to the merged
  // accentColor (= the editorScheme value when present), matching the
  // pre-split behavior.
  const Rgba merged_accent = pick(def, {"accentColor"}, def_acc);
  p.ui_accent     = def.chrome_accent_color.empty()
                  ? merged_accent
                  : parse_or(def.chrome_accent_color, merged_accent);
  p.editor_accent = def.editor_accent_color.empty()
                  ? p.ui_accent
                  : parse_or(def.editor_accent_color, p.ui_accent);
  p.accent        = p.ui_accent;  // alias for css.cpp / apply.cpp backward compat

  // --- UI chrome ---
  p.secondary_bg = pick(def, {"secondaryBackground", "headerColor", "baseBackground"}, p.window_bg);
  p.selection_bg = pick(def, {"selectionBackground"}, p.accent);
  // Alpha-aware selection bg: selectionBackgroundTransparent is "#rrggbbaa"
  // and parses with non-255 alpha; falls back to opaque selection_bg.
  p.selection_bg_alpha = pick(def, {"selectionBackgroundTransparent"}, p.selection_bg);
  p.selection_fg = pick(def, {"selectionForeground"}, p.text);
  p.button_bg    = pick(def, {"buttonColor", "secondaryBackground"}, p.secondary_bg);
  p.button_fg    = pick(def, {"buttonFont", "foregroundColor"}, p.text);
  p.border       = pick(def, {"borderColor", "secondaryBackground"}, p.secondary_bg);
  p.line_number  = pick(def, {"lineNumberColor", "comments"}, def_mut);
  p.caret_row    = pick(def, {"caretRow", "highlightColor"}, p.secondary_bg);
  p.disabled     = pick(def, {"disabledColor", "comments"}, def_mut);

  // --- Qt chrome (Phase 9) ---
  p.chrome_header       = pick(def, {"headerColor"}, p.secondary_bg);
  p.chrome_contrast     = pick(def, {"contrastColor", "codeBlock"}, p.secondary_bg);
  p.chrome_completion_bg= pick(def, {"completionWindowBackground", "codeBlock"}, p.secondary_bg);
  p.chrome_popup_mask   = pick(def, {"popupMask"}, p.window_bg);
  p.accent_contrast     = pick(def, {"accentContrastColor"}, p.text);

  // --- Accent transparency utilities (Phase 10) ---
  // The transparent accent variants are intended for editor/listing hover
  // and halo overlays, so fall back to editor_accent rather than ui_accent.
  p.accent_a35 = pick(def, {"accentColorTransparent"},      p.editor_accent);
  p.accent_a60 = pick(def, {"accentColorLessTransparent"},  p.editor_accent);
  p.accent_a16 = pick(def, {"accentColorMoreTransparent"},  p.editor_accent);

  // --- listing / syntax ---
  p.lst_bg       = p.base_bg;
  p.lst_text     = pick(def, {"foregroundColor"}, def_fg);
  p.lst_comment  = pick(def, {"comments"}, def_mut);
  // keywordColor is the primary mnemonic color; falling back to ui_accent
  // (rather than merged accentColor) keeps chrome and listing visually
  // distinct for themes like Echidna where they differ.
  p.lst_keyword  = pick(def, {"keywordColor", "accentColor"}, p.ui_accent);
  p.lst_string   = pick(def, {"stringColor"}, p.ui_accent);
  // Numbers / xrefs / errors use the editor accent (mint for Echidna)
  // because the listing is the editor surface. Only constantColor is
  // consulted; merged accentColor is intentionally not in this chain to
  // preserve the chrome/editor accent split.
  p.lst_number   = pick(def, {"constantColor"}, p.editor_accent);
  p.lst_name     = pick(def, {"classNameColor", "foregroundColor"}, p.text);
  // keyColor is the primary register color; falling back to ui_accent
  // (via the p.accent alias) keeps the chrome/editor split intact.
  p.lst_register = pick(def, {"keyColor", "accentColor"}, p.accent);
  // Phase 2 new listing roles
  p.lst_info     = pick(def, {"infoForeground"}, p.text);
  p.lst_unused   = pick(def, {"unusedColor", "disabledColor"}, p.disabled);

  // --- Selection / search / breakpoints (Phase 3) ---
  p.search_bg       = pick(def, {"searchBackground"}, p.editor_accent);
  p.search_fg       = pick(def, {"searchForeground"}, p.selection_fg);
  p.ident_highlight = pick(def, {"identifierHighlight", "highlightColor"}, p.caret_row);
  p.bp_inactive     = pick(def, {"breakpointColor"}, p.secondary_bg);
  p.bp_active       = pick(def, {"breakpointActiveColor"}, p.editor_accent);

  // --- Graph view (Phase 4) ---
  p.graph_bg_top      = pick(def, {"baseBackground"}, p.window_bg);
  p.graph_bg_bottom   = pick(def, {"secondaryBackground", "codeBlock"}, p.secondary_bg);
  p.graph_node_title  = pick(def, {"foregroundColor"}, p.text);
  p.graph_edge_normal = p.editor_accent;
  p.graph_edge_yes    = p.editor_accent;

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
