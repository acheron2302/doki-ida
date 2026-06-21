//----------------------------------------------------------------------------
// Doki Theme - IdaPalette: the concrete set of colors IDA needs, derived from
// a doki definition's palette. Single source of truth shared by the CSS
// generator (Phase 4) and the SDK listing/nav colorizer (Phase 8).
//----------------------------------------------------------------------------
#pragma once

#include "doki/color.h"
#include "doki/theme.h"

namespace doki
{

struct IdaPalette
{
  bool dark = true;

  // --- Qt / UI chrome roles ---
  Rgba window_bg;        // overall window background
  Rgba base_bg;          // editor / listing background
  Rgba secondary_bg;     // panels, headers, toolbars
  Rgba text;             // primary foreground text
  Rgba selection_bg;     // selection background (opaque)
  Rgba selection_bg_alpha; // selection background, alpha-aware (Phase 3)
  Rgba selection_fg;     // selection foreground
  Rgba ui_accent;        // chrome accent (base accentColor) -- Phase 1 split
  Rgba editor_accent;    // listing/decompiler accent (editorScheme override) -- Phase 1 split
  Rgba accent;           // alias for ui_accent (backward compat for css.cpp)
  Rgba button_bg;        // button background
  Rgba button_fg;        // button text
  Rgba border;           // borders / separators
  Rgba line_number;      // line numbers / gutter
  Rgba caret_row;        // current line highlight
  Rgba disabled;         // disabled / muted text
  Rgba error;
  Rgba warning;
  Rgba success;
  Rgba surface;
  Rgba surface_alt;
  Rgba surface_raised;

  // --- Qt chrome (Phase 9) ---
  Rgba chrome_header;        // headerColor (toolbar/menubar bg)
  Rgba chrome_contrast;      // contrastColor (raised surfaces)
  Rgba chrome_completion_bg; // completionWindowBackground
  Rgba chrome_popup_mask;    // popupMask (translucent popup mask)
  Rgba accent_contrast;      // accentContrastColor (text painted on accent)

  // --- Accent transparency utilities (Phase 10) ---
  Rgba accent_a35;  // accentColorTransparent (#rrggbbaa, alpha ~35%)
  Rgba accent_a60;  // accentColorLessTransparent
  Rgba accent_a16;  // accentColorMoreTransparent

  // --- Disassembly listing roles ---
  Rgba lst_bg;        // listing background
  Rgba lst_text;      // default instruction text
  Rgba lst_comment;   // comments
  Rgba lst_keyword;   // mnemonics / keywords
  Rgba lst_string;    // string literals
  Rgba lst_number;    // numeric constants
  Rgba lst_name;      // names / identifiers
  Rgba lst_register;  // registers
  Rgba lst_info;      // infoForeground -- imports, segments, collapsed (Phase 2)
  Rgba lst_unused;    // unusedColor -- unnamed/dummy bytes (Phase 2)
  Rgba addr_expr;
  Rgba patched_bytes;
  Rgba unsaved_changes;
  Rgba pfx_func, pfx_insn, pfx_data, pfx_extern, pfx_unexplored, pfx_libfunc;
  Rgba pfx_hidden, pfx_lumina, pfx_current_line, pfx_current_item;
  Rgba highlight[8];
  Rgba ovl_current_line, ovl_trace, ovl_trace_ovl, ovl_bookmark;
  Rgba ovl_extra[16];

  // --- Selection / search / breakpoints (Phase 3) ---
  Rgba search_bg;       // searchBackground
  Rgba search_fg;       // searchForeground
  Rgba ident_highlight; // identifierHighlight -> same-name-under-cursor
  Rgba bp_active;       // breakpointActiveColor
  Rgba bp_inactive;     // breakpointColor

  // --- Graph view (Phase 4) ---
  Rgba graph_bg_top;      // graphBgTop
  Rgba graph_bg_bottom;   // graphBgBottom
  Rgba graph_node_title;  // foregroundColor for graph node titles
  Rgba graph_title_normal, graph_title_selected, graph_title_current;
  Rgba graph_frame_selected, graph_frame_group, graph_high1, graph_high2, graph_foreign;
  Rgba graph_edge_no, graph_edge_high, graph_edge_selected;
  Rgba graph_edge_normal; // normal graph edge
  Rgba graph_edge_yes;    // "yes" / taken branch

  // --- Navigation band ---
  Rgba nav_start;     // gradient start
  Rgba nav_stop;      // gradient stop
  Rgba nav_highlight; // highlighted ranges
  Rgba nav_lib_function, nav_function, nav_code, nav_data, nav_undefined, nav_extern, nav_lumina_function;
  Rgba nav_hl_lib_function, nav_hl_function, nav_hl_code, nav_hl_data, nav_hl_undefined, nav_hl_extern, nav_hl_lumina_function;
  Rgba nav_hl_outline, nav_error, nav_gap, nav_cursor, nav_auto_analysis_cursor;

  // --- Diff / merge ---
  Rgba diff_inserted, diff_deleted, diff_modified, diff_conflict;
  Rgba diff_pick, diff_leave, diff_conflict_bg, diff_boundary;

  // --- Widget roles ---
  Rgba chooser_highlight, chooser_highlight_selected, chooser_cut, chooser_cut_selected;
  Rgba log_fg, log_bg;
  Rgba xref_bg, xref_search, xref_selected, xref_branch;
  Rgba jump_bg, jump_selected_bg, jump_selected_fg, jump_button_hover, jump_button_checked;
  Rgba cpu_label;
  Rgba graph_mini_fog, graph_mini_crosshair;
  Rgba xg_node_bg, xg_node_import, xg_node_code, xg_node_data, xg_node_border, xg_node_text, xg_edge, xg_view_bg, xg_path;
  Rgba pathfinder_drag, pathfinder_delete;
};

// Build a fully-populated palette from a definition. Every role is guaranteed
// to be set: missing keys fall back to related keys, then to dark/light
// defaults, so no role is ever left empty.
IdaPalette map_theme(const DokiThemeDefinition &def);

} // namespace doki
