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

static Rgba readable(Rgba fg, const Rgba &bg, bool dark, double ratio = 3.0)
{
  return ensure_contrast(fg, bg, ratio,
                         dark ? ContrastPreference::Lighten
                              : ContrastPreference::Darken);
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
  p.error        = readable(pick(def, {"errorColor"}, Rgba{0xff,0x5f,0x70,255}), p.base_bg, def.dark, 3.0);
  p.warning      = readable(pick(def, {"warningColor", "constantColor"}, Rgba{0xff,0xc6,0x6d,255}), p.base_bg, def.dark, 3.0);
  p.success      = readable(pick(def, {"successColor", "stringColor"}, Rgba{0x75,0xd6,0x9c,255}), p.base_bg, def.dark, 3.0);
  p.surface      = p.base_bg;
  p.surface_alt  = pick(def, {"codeBlock", "secondaryBackground"}, p.secondary_bg);
  p.surface_raised = pick(def, {"contrastColor", "completionWindowBackground"}, p.surface_alt);

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
  p.lst_text     = readable(p.lst_text, p.lst_bg, def.dark, 4.0);
  p.lst_comment  = readable(p.lst_comment, p.lst_bg, def.dark, 3.0);
  p.lst_keyword  = readable(p.lst_keyword, p.lst_bg, def.dark, 3.0);
  p.lst_string   = readable(p.lst_string, p.lst_bg, def.dark, 3.0);
  p.lst_number   = readable(p.lst_number, p.lst_bg, def.dark, 3.0);
  p.lst_name     = readable(p.lst_name, p.lst_bg, def.dark, 3.0);
  p.lst_register = readable(p.lst_register, p.lst_bg, def.dark, 3.0);
  p.lst_info     = readable(p.lst_info, p.lst_bg, def.dark, 3.0);
  p.lst_unused   = readable(p.lst_unused, p.lst_bg, def.dark, 2.4);
  p.addr_expr      = readable(pick(def, {"classNameColor", "keyColor"}, p.lst_register), p.lst_bg, def.dark, 3.0);
  p.patched_bytes  = readable(pick(def, {"diff.modified", "warningColor", "constantColor"}, p.warning), p.lst_bg, def.dark, 3.0);
  p.unsaved_changes= readable(pick(def, {"diff.inserted", "successColor", "stringColor"}, p.success), p.lst_bg, def.dark, 3.0);
  p.pfx_func = p.lst_name; p.pfx_insn = p.lst_keyword; p.pfx_data = p.lst_string;
  p.pfx_extern = p.lst_register; p.pfx_unexplored = p.lst_unused; p.pfx_libfunc = p.lst_info;
  p.pfx_hidden = p.lst_comment; p.pfx_lumina = p.success;
  p.pfx_current_line = with_alpha(p.editor_accent, 120);
  p.pfx_current_item = p.caret_row;

  // --- Selection / search / breakpoints (Phase 3) ---
  p.search_bg       = pick(def, {"searchBackground"}, p.editor_accent);
  p.search_fg       = pick(def, {"searchForeground"}, p.selection_fg);
  p.ident_highlight = pick(def, {"identifierHighlight", "highlightColor"}, p.caret_row);
  p.bp_inactive     = pick(def, {"breakpointColor"}, p.secondary_bg);
  p.bp_active       = pick(def, {"breakpointActiveColor"}, p.editor_accent);
  p.pfx_current_item = p.ident_highlight;
  p.search_fg       = readable(p.search_fg, p.search_bg, def.dark, 3.0);
  p.selection_fg    = readable(p.selection_fg, p.selection_bg, def.dark, 3.0);
  p.highlight[0] = with_alpha(p.search_bg, p.search_bg.a == 255 ? 210 : p.search_bg.a);
  p.highlight[1] = with_alpha(p.ident_highlight, p.ident_highlight.a == 255 ? 160 : p.ident_highlight.a);
  p.highlight[2] = with_alpha(p.editor_accent, 120);
  p.highlight[3] = with_alpha(p.ui_accent, 115);
  p.highlight[4] = with_alpha(p.lst_string, 110);
  p.highlight[5] = with_alpha(p.lst_number, 110);
  p.highlight[6] = with_alpha(p.lst_register, 110);
  p.highlight[7] = with_alpha(p.warning, 120);
  p.ovl_current_line = with_alpha(p.caret_row, 90);
  p.ovl_trace = with_alpha(p.success, 95);
  p.ovl_trace_ovl = with_alpha(p.success, 150);
  p.ovl_bookmark = with_alpha(p.warning, 170);
  for ( int i = 0; i < 16; ++i )
  {
    const Rgba seed = (i % 4 == 0) ? p.editor_accent : (i % 4 == 1) ? p.lst_string : (i % 4 == 2) ? p.lst_number : p.lst_register;
    p.ovl_extra[i] = with_alpha(seed, (uint8_t)(55 + (i % 4) * 20));
  }

  // --- Graph view (Phase 4) ---
  p.graph_bg_top      = pick(def, {"baseBackground"}, p.window_bg);
  p.graph_bg_bottom   = pick(def, {"secondaryBackground", "codeBlock"}, p.secondary_bg);
  p.graph_node_title  = pick(def, {"foregroundColor"}, p.text);
  p.graph_edge_normal = p.editor_accent;
  p.graph_edge_yes    = p.editor_accent;
  p.graph_title_normal = p.surface_raised;
  p.graph_title_selected = blend(p.surface_raised, p.editor_accent, 0.35);
  p.graph_title_current = blend(p.surface_raised, p.editor_accent, 0.55);
  p.graph_frame_selected = p.editor_accent;
  p.graph_frame_group = p.ui_accent;
  p.graph_high1 = p.highlight[0];
  p.graph_high2 = p.highlight[1];
  p.graph_foreign = p.lst_unused;
  p.graph_edge_no = p.error;
  p.graph_edge_high = p.warning;
  p.graph_edge_selected = p.lst_name;

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
  p.nav_function = p.lst_name; p.nav_lib_function = p.lst_info; p.nav_code = p.lst_keyword;
  p.nav_data = p.lst_string; p.nav_undefined = p.lst_unused; p.nav_extern = p.lst_register;
  p.nav_lumina_function = p.success;
  p.nav_hl_function = blend(p.nav_function, p.nav_highlight, 0.45);
  p.nav_hl_lib_function = blend(p.nav_lib_function, p.nav_highlight, 0.45);
  p.nav_hl_code = blend(p.nav_code, p.nav_highlight, 0.45);
  p.nav_hl_data = blend(p.nav_data, p.nav_highlight, 0.45);
  p.nav_hl_undefined = blend(p.nav_undefined, p.nav_highlight, 0.45);
  p.nav_hl_extern = blend(p.nav_extern, p.nav_highlight, 0.45);
  p.nav_hl_lumina_function = blend(p.nav_lumina_function, p.nav_highlight, 0.45);
  p.nav_hl_outline = p.editor_accent;
  p.nav_error = p.error;
  p.nav_gap = p.lst_bg;
  p.nav_cursor = p.editor_accent;
  p.nav_auto_analysis_cursor = p.warning;

  p.diff_inserted = with_alpha(pick(def, {"diff.inserted", "successColor", "stringColor"}, p.success), 120);
  p.diff_deleted = with_alpha(pick(def, {"diff.deleted", "errorColor"}, p.error), 120);
  p.diff_modified = with_alpha(pick(def, {"diff.modified", "warningColor", "constantColor"}, p.warning), 120);
  p.diff_conflict = with_alpha(pick(def, {"diff.conflict", "errorColor"}, p.error), 150);
  p.diff_pick = p.diff_inserted; p.diff_leave = p.diff_deleted;
  p.diff_conflict_bg = p.diff_conflict; p.diff_boundary = p.editor_accent;

  p.chooser_highlight = p.highlight[1]; p.chooser_highlight_selected = p.selection_bg;
  p.chooser_cut = p.lst_unused; p.chooser_cut_selected = readable(p.lst_unused, p.selection_bg, def.dark, 3.0);
  p.log_fg = readable(p.text, p.surface_alt, def.dark, 4.0); p.log_bg = p.surface_alt;
  p.xref_bg = p.surface_alt; p.xref_search = p.search_bg; p.xref_selected = p.selection_bg; p.xref_branch = p.editor_accent;
  p.jump_bg = p.surface_alt; p.jump_selected_bg = p.selection_bg; p.jump_selected_fg = p.selection_fg;
  p.jump_button_hover = p.highlight[2]; p.jump_button_checked = p.highlight[3];
  p.cpu_label = p.lst_info;
  p.graph_mini_fog = with_alpha(p.lst_bg, 160); p.graph_mini_crosshair = p.editor_accent;
  p.xg_node_bg = p.surface_raised; p.xg_node_import = blend(p.surface_raised, p.lst_info, 0.25);
  p.xg_node_code = blend(p.surface_raised, p.lst_keyword, 0.25); p.xg_node_data = blend(p.surface_raised, p.lst_string, 0.25);
  p.xg_node_border = p.border; p.xg_node_text = p.text; p.xg_edge = p.editor_accent; p.xg_view_bg = p.lst_bg; p.xg_path = p.warning;
  p.pathfinder_drag = p.editor_accent; p.pathfinder_delete = p.error;

  return p;
}

} // namespace doki
