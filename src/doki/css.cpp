//----------------------------------------------------------------------------
// Doki Theme - theme.css generation implementation.
//----------------------------------------------------------------------------
#include "doki/css.h"

#include <sstream>

namespace doki
{

// rgba() string with an explicit alpha (0..1), independent of the color's own.
static std::string rgba_alpha(const Rgba &c, double alpha)
{
  char buf[48];
  std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, %.3f)", c.r, c.g, c.b, alpha);
  return buf;
}

static std::string anchor_to_position(const std::string &anchor)
{
  if ( anchor == "center" ) return "center center";
  if ( anchor == "left" )   return "left center";
  if ( anchor == "right" )  return "right center";
  if ( anchor == "top" )    return "center top";
  if ( anchor == "bottom" ) return "center bottom";
  return "bottom right"; // default for a corner mascot
}

std::string generate_theme_css(const DokiThemeDefinition &def,
                               const IdaPalette &pal,
                               const CssOptions &opt)
{
  std::ostringstream o;
  const char *base = pal.dark ? "dark" : "default";
  // The sticker is rendered by the Qt overlay (DokiOverlayManager) on top of
  // the view, not by CSS. Wallpaper (if any) is CSS-driven: it is applied
  // directly on CustomIDAMemo per the official Hex-Rays blog approach, and
  // its transparency comes from the per-pixel alpha baked into the
  // upstream Doki PNG. There is no CSS opacity and no Qt wallpaper layer.
  const bool wall = opt.include_wallpaper && !opt.wallpaper_file.empty();
  // When an image sits behind the listing, the per-line background must be
  // transparent so the image shows through.
  const bool transparent_bg = wall;

  o << "/* doki-theme: " << def.displayName
    << " (" << def.group << ") - generated, do not edit by hand */\n";
  o << "@importtheme \"" << base << "\";\n\n";

  // Palette variables.
  // The listing/decompiler surfaces use editor_accent (the upstream
  // BuildThemes.ts split); chrome surfaces use ui_accent.
  o << "@def doki_bg           " << to_css_hex(pal.lst_bg)            << ";\n";
  o << "@def doki_window_bg    " << to_css_hex(pal.window_bg)         << ";\n";
  o << "@def doki_fg           " << to_css_hex(pal.lst_text)          << ";\n";
  o << "@def doki_comment      " << to_css_hex(pal.lst_comment)       << ";\n";
  o << "@def doki_keyword      " << to_css_hex(pal.lst_keyword)       << ";\n";
  o << "@def doki_string       " << to_css_hex(pal.lst_string)        << ";\n";
  o << "@def doki_number       " << to_css_hex(pal.lst_number)        << ";\n";
  o << "@def doki_name         " << to_css_hex(pal.lst_name)          << ";\n";
  o << "@def doki_register     " << to_css_hex(pal.lst_register)      << ";\n";
  o << "@def doki_info         " << to_css_hex(pal.lst_info)          << ";\n";
  o << "@def doki_unused       " << to_css_hex(pal.lst_unused)        << ";\n";
  o << "@def doki_eaccent      " << to_css_hex(pal.editor_accent)     << ";\n";
  o << "@def doki_lineno       " << to_css_hex(pal.line_number)       << ";\n";
  o << "@def doki_sel_bg_alpha " << to_css_rgba(pal.selection_bg_alpha) << ";\n";
  o << "@def doki_selection_bg " << to_css_hex(pal.selection_bg)       << ";\n";
  o << "@def doki_search_bg    " << to_css_hex(pal.search_bg)         << ";\n";
  o << "@def doki_ident_hi     " << to_css_rgba(pal.ident_highlight)  << ";\n";
  o << "@def doki_bpt_active   " << to_css_rgba(pal.bp_active)        << ";\n";
  o << "@def doki_bpt_inactive " << to_css_rgba(pal.bp_inactive)      << ";\n";
  o << "@def doki_border       " << to_css_hex(pal.border)            << ";\n";
  o << "@def doki_chrome_hdr   " << to_css_hex(pal.chrome_header)     << ";\n";
  o << "@def doki_chrome_cntr  " << to_css_hex(pal.chrome_contrast)   << ";\n";
  o << "@def doki_chrome_cmp   " << to_css_hex(pal.chrome_completion_bg) << ";\n";
  o << "@def doki_error        " << to_css_hex(pal.error)             << ";\n";
  o << "@def doki_warning      " << to_css_hex(pal.warning)           << ";\n";
  o << "@def doki_success      " << to_css_hex(pal.success)           << ";\n";
  o << "@def doki_surface      " << to_css_hex(pal.surface_alt)       << ";\n";
  o << "@def doki_surface_hi   " << to_css_hex(pal.surface_raised)    << ";\n";
  o << "@def doki_sel_fg       " << to_css_hex(pal.selection_fg)      << ";\n";
  o << "@def doki_search_fg    " << to_css_hex(pal.search_fg)         << ";\n";
  o << "@def doki_addr_expr    " << to_css_hex(pal.addr_expr)         << ";\n";
  o << "@def doki_patched      " << to_css_hex(pal.patched_bytes)     << ";\n";
  o << "@def doki_unsaved      " << to_css_hex(pal.unsaved_changes)   << ";\n";
  o << "@def doki_hl3          " << to_css_rgba(pal.highlight[2])     << ";\n";
  o << "@def doki_hl4          " << to_css_rgba(pal.highlight[3])     << ";\n";
  o << "@def doki_hl5          " << to_css_rgba(pal.highlight[4])     << ";\n";
  o << "@def doki_hl6          " << to_css_rgba(pal.highlight[5])     << ";\n";
  o << "@def doki_hl7          " << to_css_rgba(pal.highlight[6])     << ";\n";
  o << "@def doki_hl8          " << to_css_rgba(pal.highlight[7])     << ";\n";
  o << "@def doki_diff_ins     " << to_css_rgba(pal.diff_inserted)    << ";\n";
  o << "@def doki_diff_del     " << to_css_rgba(pal.diff_deleted)     << ";\n";
  o << "@def doki_diff_mod     " << to_css_rgba(pal.diff_modified)    << ";\n";
  o << "@def doki_diff_conf    " << to_css_rgba(pal.diff_conflict)    << ";\n";
  o << "@def doki_graph_title  " << to_css_hex(pal.graph_title_normal) << ";\n";
  o << "@def doki_graph_sel    " << to_css_hex(pal.graph_title_selected) << ";\n";
  o << "@def doki_graph_cur    " << to_css_hex(pal.graph_title_current) << ";\n";
  o << "@def doki_graph_frame  " << to_css_hex(pal.graph_frame_selected) << ";\n";
  o << "@def doki_nav_hi       " << to_css_hex(pal.nav_highlight)      << ";\n\n";

  // Broad Qt / IDA widget background coverage (Phase 11). Modeled on
  // IDA 9.4 dark/theme.css so IDA chrome picks up the doki palette
  // instead of inheriting the importtheme() defaults. The rules below
  // are intentionally emitted BEFORE the listing/pseudocode blocks so
  // they do not interfere with listing-specific overrides; transparency
  // overrides for wallpaper mode are re-emitted at the end.
  o << "/* ---- broad Qt / IDA chrome backgrounds ---- */\n";
  o << "QWidget\n{\n";
  o << "    background-color: ${doki_window_bg};\n";
  o << "    color: ${doki_fg};\n";
  o << "}\n\n";

  o << "QTextEdit, QPlainTextEdit\n{\n";
  o << "    background-color: ${doki_bg};\n";
  o << "    border: 1px solid ${doki_border};\n";
  o << "}\n\n";

  o << "DockWidgetTitle[active=\"true\"], DockAreaDragTitle\n{\n";
  o << "    background-color: ${doki_chrome_hdr};\n";
  o << "}\n\n";

  o << "DockWidget > QWidget > QAbstractButton\n{\n";
  o << "    background-color: ${doki_bg};\n";
  o << "}\n\n";

  o << "QMenu::item:selected\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "}\n\n";

  o << "QTabBar::tab\n{\n";
  o << "    background-color: ${doki_chrome_hdr};\n";
  o << "}\n\n";
  o << "QTabBar::tab:selected\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "}\n\n";

  o << "QTableView\n{\n";
  o << "    background-color: ${doki_bg};\n";
  o << "    border: 1px solid ${doki_border};\n";
  o << "}\n\n";

  o << "QTableCornerButton::section\n{\n";
  o << "    background-color: ${doki_bg};\n";
  o << "}\n\n";

  o << "QScrollBar\n{\n";
  o << "    background-color: ${doki_window_bg};\n";
  o << "}\n\n";

  o << "QScrollBar::handle\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "}\n\n";

  o << "QPushButton:default\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "}\n\n";

  o << "QComboBox:!editable\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "}\n\n";

  // Selection rules. Each pseudo-state lives in its own rule block so the
// Qt stylesheet cascade does not have to evaluate all four pseudo-states
// together on every selection paint.
  o << "QTreeView::item:selected\n{\n";
  o << "    background-color: ${doki_selection_bg};\n";
  o << "    color: ${doki_sel_fg};\n";
  o << "}\n\n";
  o << "QListView::item:selected\n{\n";
  o << "    background-color: ${doki_selection_bg};\n";
  o << "    color: ${doki_sel_fg};\n";
  o << "}\n\n";
  o << "QTableView::item:selected\n{\n";
  o << "    background-color: ${doki_selection_bg};\n";
  o << "    color: ${doki_sel_fg};\n";
  o << "}\n\n";
  o << "QTreeView::branch:selected\n{\n";
  o << "    background-color: ${doki_selection_bg};\n";
  o << "    color: ${doki_sel_fg};\n";
  o << "}\n\n";

  // Disassembly and pseudocode listings both use CustomIDAMemo. When a
  // wallpaper is layered behind the listing, the per-line background must
  // be transparent so the image shows through.
  // See https://hex-rays.com/blog/ui-candy for the official approach.
  o << "CustomIDAMemo\n{\n";
  if ( transparent_bg )
    o << "    qproperty-line-bg-default            : " << rgba_alpha(pal.lst_bg, 0.0) << ";\n";
  else
    o << "    qproperty-line-bg-default            : ${doki_bg};\n";

  // ── foreground: text & mnemonics ──
  o << "    qproperty-line-fg-default            : ${doki_fg};\n";
  o << "    qproperty-line-fg-insn               : ${doki_keyword};\n";
  o << "    qproperty-line-fg-keyword            : ${doki_register};\n";
  o << "    qproperty-line-fg-asm-directive      : ${doki_keyword};\n";
  o << "    qproperty-line-fg-macro              : ${doki_keyword};\n";

  // ── foreground: registers & literals ──
  o << "    qproperty-line-fg-register-name      : ${doki_register};\n";
  o << "    qproperty-line-fg-strlit-in-insn     : ${doki_string};\n";
  o << "    qproperty-line-fg-strlit-in-data     : ${doki_string};\n";
  o << "    qproperty-line-fg-charlit-in-insn    : ${doki_string};\n";
  o << "    qproperty-line-fg-charlit-in-data    : ${doki_string};\n";
  o << "    qproperty-line-fg-numlit-in-insn     : ${doki_number};\n";
  o << "    qproperty-line-fg-numlit-in-data     : ${doki_number};\n";

  // ── foreground: comments ──
  o << "    qproperty-line-fg-regular-comment    : ${doki_comment};\n";
  o << "    qproperty-line-fg-repeatable-comment : ${doki_comment};\n";
  o << "    qproperty-line-fg-automatic-comment  : ${doki_comment};\n";
  o << "    qproperty-line-fg-extra-line         : ${doki_comment};\n";
  o << "    qproperty-line-fg-hidden             : ${doki_comment};\n";

  // ── foreground: names ──
  o << "    qproperty-line-fg-regular-data-name  : ${doki_name};\n";
  o << "    qproperty-line-fg-dummy-data-name    : ${doki_name};\n";
  o << "    qproperty-line-fg-code-name          : ${doki_name};\n";
  o << "    qproperty-line-fg-dummy-code-name    : ${doki_name};\n";
  o << "    qproperty-line-fg-demangled-name     : ${doki_name};\n";
  o << "    qproperty-line-fg-libfunc            : ${doki_number};\n";
  o << "    qproperty-line-fg-locvar             : ${doki_name};\n";

  // ── foreground: metadata (info/unused) ──
  o << "    qproperty-line-fg-import-name        : ${doki_info};\n";
  o << "    qproperty-line-fg-segment-name       : ${doki_info};\n";
  o << "    qproperty-line-fg-collapsed-line     : ${doki_info};\n";
  o << "    qproperty-line-fg-unknown-name       : ${doki_unused};\n";
  o << "    qproperty-line-fg-dummy-unknown-name : ${doki_unused};\n";

  // ── foreground: cross-references ──
  o << "    qproperty-line-fg-code-xref          : ${doki_eaccent};\n";
  o << "    qproperty-line-fg-code-xref-to-tail  : ${doki_eaccent};\n";
  o << "    qproperty-line-fg-data-xref          : ${doki_name};\n";
  o << "    qproperty-line-fg-data-xref-to-tail  : ${doki_name};\n";

  // ── foreground: prefix & opcode (gutter-gray) ──
  o << "    qproperty-line-fg-line-prefix        : ${doki_lineno};\n";
  o << "    qproperty-line-fg-opcode-byte        : ${doki_lineno};\n";

  // ── foreground: misc ──
  o << "    qproperty-line-fg-addr-expr          : ${doki_addr_expr};\n";
  o << "    qproperty-line-fg-patched-bytes      : ${doki_patched};\n";
  o << "    qproperty-line-fg-unsaved-changes    : ${doki_unsaved};\n";
  o << "    qproperty-line-fg-punctuation        : ${doki_lineno};\n";
  o << "    qproperty-line-fg-alt-opnd           : ${doki_fg};\n";
  o << "    qproperty-line-fg-void-opnd          : ${doki_error};\n";
  o << "    qproperty-line-fg-error              : ${doki_error};\n";
  o << "    qproperty-line-fg-lumina             : ${doki_success};\n";

  // ── prefix/gutter roles ──
  o << "    qproperty-line-pfx-libfunc           : " << to_css_hex(pal.pfx_libfunc) << ";\n";
  o << "    qproperty-line-pfx-func              : " << to_css_hex(pal.pfx_func) << ";\n";
  o << "    qproperty-line-pfx-insn              : " << to_css_hex(pal.pfx_insn) << ";\n";
  o << "    qproperty-line-pfx-data              : " << to_css_hex(pal.pfx_data) << ";\n";
  o << "    qproperty-line-pfx-unexplored        : " << to_css_hex(pal.pfx_unexplored) << ";\n";
  o << "    qproperty-line-pfx-extern            : " << to_css_hex(pal.pfx_extern) << ";\n";
  o << "    qproperty-line-pfx-current-line      : " << to_css_rgba(pal.pfx_current_line) << ";\n";
  o << "    qproperty-line-pfx-hidden-line       : " << to_css_hex(pal.pfx_hidden) << ";\n";
  o << "    qproperty-line-pfx-lumina            : " << to_css_hex(pal.pfx_lumina) << ";\n";

  // ── selection (honor the AA byte -> translucent) ──
  o << "    qproperty-line-bg-selected           : ${doki_sel_bg_alpha};\n";

  // ── highlight backgrounds (8 user highlight slots) ──
  // Theme slots 1 and 2; slots 3-8 keep base defaults (would otherwise
  // inherit the stock yellow, which clashes with dark themes).
  o << "    qproperty-line-bg-highlight          : ${doki_search_bg};\n";
  o << "    qproperty-line-fg-highlight          : ${doki_search_fg};\n";
  o << "    qproperty-line-bg-highlight-2        : ${doki_ident_hi};\n";
  o << "    qproperty-line-bg-highlight-3        : ${doki_hl3};\n";
  o << "    qproperty-line-bg-highlight-4        : ${doki_hl4};\n";
  o << "    qproperty-line-bg-highlight-5        : ${doki_hl5};\n";
  o << "    qproperty-line-bg-highlight-6        : ${doki_hl6};\n";
  o << "    qproperty-line-bg-highlight-7        : ${doki_hl7};\n";
  o << "    qproperty-line-bg-highlight-8        : ${doki_hl8};\n";

  // ── current-line overlay (caretRow, subtle) ──
  o << "    qproperty-line-bgovl-current-line    : " << to_css_rgba(pal.ovl_current_line) << ";\n";
  o << "    qproperty-line-bgovl-current-ip      : ${doki_warning};\n";
  o << "    qproperty-line-bgovl-trace           : " << to_css_rgba(pal.ovl_trace) << ";\n";
  o << "    qproperty-line-bgovl-trace-ovl       : " << to_css_rgba(pal.ovl_trace_ovl) << ";\n";
  o << "    qproperty-line-bgovl-bookmark        : " << to_css_rgba(pal.ovl_bookmark) << ";\n";
  o << "    qproperty-bookmark-star-outline      : ${doki_warning};\n";
  for ( int i = 0; i < 16; ++i )
    o << "    qproperty-line-bgovl-extra-" << (i + 1) << "        : " << to_css_rgba(pal.ovl_extra[i]) << ";\n";
  o << "    qproperty-line-bgovl-diff-ins        : ${doki_diff_ins};\n";
  o << "    qproperty-line-bgovl-diff-del        : ${doki_diff_del};\n";
  o << "    qproperty-line-bgovl-diff-mod        : ${doki_diff_mod};\n";
  o << "    qproperty-line-bgovl-diff-conflict   : ${doki_diff_conf};\n";

  // ── current-item prefix (identifierHighlight, thin bar) ──
  o << "    qproperty-line-pfx-current-item      : ${doki_ident_hi};\n";

  // ── breakpoints (breakpointColor / breakpointActiveColor) ──
  o << "    qproperty-line-bg-bpt-enabled        : ${doki_bpt_active};\n";
  o << "    qproperty-line-bg-bpt-disabled       : ${doki_bpt_inactive};\n";
  o << "    qproperty-line-bg-bpt-unavailable    : " << rgba_alpha(pal.bp_inactive, 0.30) << ";\n";

  // ── dereferencing colors (Phase 5) ──
  o << "    qproperty-line-fg-deref-code         : ${doki_eaccent};\n";
  o << "    qproperty-line-fg-deref-data         : ${doki_name};\n";
  o << "    qproperty-line-fg-deref-rodata       : ${doki_register};\n";
  o << "    qproperty-line-fg-deref-stack        : ${doki_info};\n";
  o << "    qproperty-line-fg-deref-number       : ${doki_number};\n";
  o << "    qproperty-line-fg-deref-writable-code: ${doki_keyword};\n";

  // ── graph view (Phase 4) ──
  o << "    qproperty-graph-bg-top               : ${doki_bg};\n";
  o << "    qproperty-graph-bg-bottom            : " << to_css_hex(pal.graph_bg_bottom) << ";\n";
  o << "    qproperty-graph-node-title-normal    : ${doki_graph_title};\n";
  o << "    qproperty-graph-node-title-selected  : ${doki_graph_sel};\n";
  o << "    qproperty-graph-node-title-current   : ${doki_graph_cur};\n";
  o << "    qproperty-graph-node-shadow          : rgba(0,0,0,0.4);\n";
  o << "    qproperty-graph-node-frame-selected  : ${doki_graph_frame};\n";
  o << "    qproperty-graph-node-frame-group     : " << to_css_hex(pal.graph_frame_group) << ";\n";
  o << "    qproperty-graph-node-high1           : " << to_css_rgba(pal.graph_high1) << ";\n";
  o << "    qproperty-graph-node-high2           : " << to_css_rgba(pal.graph_high2) << ";\n";
  o << "    qproperty-graph-node-foreign         : " << to_css_hex(pal.graph_foreign) << ";\n";
  o << "    qproperty-graph-edge-normal          : ${doki_eaccent};\n";
  o << "    qproperty-graph-edge-yes             : ${doki_eaccent};\n";
  o << "    qproperty-graph-edge-no              : " << to_css_hex(pal.graph_edge_no) << ";\n";
  o << "    qproperty-graph-edge-high            : " << to_css_hex(pal.graph_edge_high) << ";\n";
  o << "    qproperty-graph-edge-selected        : " << to_css_hex(pal.graph_edge_selected) << ";\n";

  // ── caret ──
  o << "    qproperty-caret                      : ${doki_fg};\n";

  if ( wall )
  {
    // Wallpaper is painted directly on CustomIDAMemo per the official
    // Hex-Rays blog approach. Both disassembly and pseudocode listings use
    // CustomIDAMemo, so a single rule covers both views. The sticker is
    // drawn by the Qt overlay on top, so it doesn't compete with CSS.
    o << "\n    background            : ${doki_bg} url(\"$RELPATH/"
      << opt.wallpaper_file << "\");\n";
    o << "    background-repeat     : no-repeat;\n";
    o << "    background-position   : " << anchor_to_position(opt.wallpaper_anchor) << ";\n";
    o << "    background-attachment : fixed;\n";
  }
  o << "}\n\n";

  o << "CustomIDAMemo[os-dark-theme=\"true\"]\n{\n";
  o << "    qproperty-line-bg-default            : " << (transparent_bg ? rgba_alpha(pal.lst_bg, 0.0) : std::string("${doki_bg}")) << ";\n";
  o << "    qproperty-line-fg-default            : ${doki_fg};\n";
  o << "    qproperty-line-fg-error              : ${doki_error};\n";
  o << "    qproperty-line-bg-selected           : ${doki_sel_bg_alpha};\n";
  o << "    qproperty-line-bg-highlight-3        : ${doki_hl3};\n";
  o << "    qproperty-graph-node-frame-selected  : ${doki_graph_frame};\n";
  o << "}\n\n";

  o << "CustomIDAMemo[debugging=\"true\"]\n{\n";
  o << "    qproperty-line-bg-default            : ${doki_bg};\n";
  o << "    qproperty-line-bgovl-current-ip      : ${doki_warning};\n";
  o << "}\n\n";

  o << "CustomIDAMemo[hints=\"true\"]\n{\n";
  o << "    qproperty-line-bg-default            : ${doki_surface};\n";
  o << "    qproperty-line-fg-default            : ${doki_fg};\n";
  o << "    qproperty-hint-bg                    : ${doki_surface_hi};\n";
  o << "    qproperty-hint-caption-fg            : ${doki_sel_fg};\n";
  o << "    qproperty-hint-caption-bg            : ${doki_eaccent};\n";
  o << "}\n\n";

  if ( transparent_bg )
  {
    // Keep the fringe/arrows from painting over the image.
    o << "ecfringe_t  { background: transparent; }\n";
    o << "TextArrows  { background: transparent; }\n";

    if ( wall )
    {
      // Wallpaper-only: on CustomIDAMemo (blog). IDAViewHost solid,
      // pseudocode containers transparent so nothing covers the memo.
      o << "IDAViewHost { background: ${doki_bg}; }\n\n";

      o << "text_area_t { background: transparent; }\n\n";

      o << "text_area_t QWidget\n";
      o << "{\n";
      o << "    background-color: transparent;\n";
      o << "}\n\n";

      o << "text_area_t text_area_margin_widget_t\n";
      o << "{\n";
      o << "    background-color: transparent;\n";
      o << "}\n\n";
    }
  }

  // Navigation band: per-type colors (Phase 8). Per-range gradient colors
  // are still painted by the C++ colorizer in apply.cpp; this CSS block
  // ensures the block-color fallback is themed too.
  o << "navband_t\n{\n";
  o << "    background-color       : ${doki_bg};\n";
  o << "    qproperty-function     : " << to_css_hex(pal.nav_function) << ";\n";
  o << "    qproperty-lib-function : " << to_css_hex(pal.nav_lib_function) << ";\n";
  o << "    qproperty-code         : " << to_css_hex(pal.nav_code) << ";\n";
  o << "    qproperty-data         : " << to_css_hex(pal.nav_data) << ";\n";
  o << "    qproperty-undefined    : " << to_css_hex(pal.nav_undefined) << ";\n";
  o << "    qproperty-extern       : " << to_css_hex(pal.nav_extern) << ";\n";
  o << "    qproperty-lumina-function : " << to_css_hex(pal.nav_lumina_function) << ";\n";
  o << "    qproperty-hl-function  : " << to_css_hex(pal.nav_hl_function) << ";\n";
  o << "    qproperty-hl-lib-function : " << to_css_hex(pal.nav_hl_lib_function) << ";\n";
  o << "    qproperty-hl-code      : " << to_css_hex(pal.nav_hl_code) << ";\n";
  o << "    qproperty-hl-data      : " << to_css_hex(pal.nav_hl_data) << ";\n";
  o << "    qproperty-hl-undefined : " << to_css_hex(pal.nav_hl_undefined) << ";\n";
  o << "    qproperty-hl-extern    : " << to_css_hex(pal.nav_hl_extern) << ";\n";
  o << "    qproperty-hl-lumina-function : " << to_css_hex(pal.nav_hl_lumina_function) << ";\n";
  o << "    qproperty-hl-outline   : " << to_css_hex(pal.nav_hl_outline) << ";\n";
  o << "    qproperty-error        : " << to_css_hex(pal.nav_error) << ";\n";
  o << "    qproperty-cursor       : " << to_css_hex(pal.nav_cursor) << ";\n";
  o << "    qproperty-auto-analysis-cursor : " << to_css_hex(pal.nav_auto_analysis_cursor) << ";\n";
  o << "    qproperty-gap          : " << to_css_hex(pal.nav_gap) << ";\n";
  o << "}\n\n";

  o << "navband_t[os-dark-theme=\"true\"]\n{\n";
  o << "    background-color       : ${doki_bg};\n";
  o << "    qproperty-hl-outline   : ${doki_nav_hi};\n";
  o << "    qproperty-cursor       : ${doki_eaccent};\n";
  o << "}\n\n";

  // Hex-Rays pseudocode / text-area colors. The qproperty-* keys map to
  // the fields exposed by text_area_t.
  o << "text_area_t\n{\n";
  o << "    qproperty-keyword1-fg     : ${doki_keyword};\n";
  o << "    qproperty-keyword2-fg     : ${doki_eaccent};\n";
  o << "    qproperty-keyword3-fg     : ${doki_number};\n";
  o << "    qproperty-string-fg       : ${doki_string};\n";
  o << "    qproperty-comment-fg      : ${doki_comment};\n";
  o << "    qproperty-preprocessor-fg : ${doki_keyword};\n";
  o << "    qproperty-number-fg       : ${doki_number};\n";
  o << "    qproperty-user1-fg        : ${doki_info};\n";
  o << "    qproperty-user2-fg        : ${doki_name};\n";
  o << "    qproperty-user3-fg        : ${doki_register};\n";
  o << "    qproperty-user4-fg        : ${doki_eaccent};\n";
  o << "    qproperty-keyword1-weight : 75;\n";
  o << "    qproperty-keyword2-weight : 75;\n";
  o << "    qproperty-keyword3-weight : 75;\n";
  o << "    qproperty-string-weight   : 50;\n";
  o << "    qproperty-comment-italic  : true;\n";
  o << "    qproperty-preprocessor-weight : 75;\n";
  o << "    qproperty-number-weight   : 75;\n";
  o << "    qproperty-user1-weight    : 50;\n";
  o << "    qproperty-user2-weight    : 50;\n";
  o << "    qproperty-user3-weight    : 50;\n";
  o << "    qproperty-user4-weight    : 50;\n";
  o << "    qproperty-match-line-bg   : " << rgba_alpha(pal.search_bg, 0.25) << ";\n";
  o << "    qproperty-match-text-bg   : ${doki_search_bg};\n";
  o << "}\n\n";

  o << "text_area_t[os-dark-theme=\"true\"]\n{\n";
  o << "    qproperty-keyword1-fg     : ${doki_keyword};\n";
  o << "    qproperty-string-fg       : ${doki_string};\n";
  o << "    qproperty-match-text-bg   : ${doki_search_bg};\n";
  o << "}\n\n";

  o << "text_area_t text_area_margin_widget_t\n{\n";
  o << "    color                  : ${doki_lineno};\n";
  o << "    qproperty-header-color : ${doki_fg};\n";
  o << "}\n\n";

  // Decompiler semantic colors exposed via syntax_color_t. These cover the
  // distinct categories Hex-Rays emits (function names, library funcs,
  // imports, data references, etc.).
  o << "syntax_color_t\n{\n";
  o << "    qproperty-function-name-color        : ${doki_name};\n";
  o << "    qproperty-library-function-color     : ${doki_name};\n";
  o << "    qproperty-imported-function-color    : ${doki_info};\n";
  o << "    qproperty-data-name-color            : ${doki_name};\n";
  o << "    qproperty-regular-data-name-color    : ${doki_name};\n";
  o << "    qproperty-string-literal-color       : ${doki_string};\n";
  o << "    qproperty-data-string-color          : ${doki_string};\n";
  o << "    qproperty-numeric-constant-color     : ${doki_number};\n";
  o << "    qproperty-data-numeric-color         : ${doki_number};\n";
  o << "    qproperty-keyword-color              : ${doki_keyword};\n";
  o << "    qproperty-symbol-color               : ${doki_fg};\n";
  o << "    qproperty-register-color             : ${doki_register};\n";
  o << "    qproperty-local-name-color           : ${doki_name};\n";
  o << "    qproperty-code-reference-color       : ${doki_eaccent};\n";
  o << "    qproperty-data-reference-color       : ${doki_number};\n";
  o << "    qproperty-prefix-color               : ${doki_lineno};\n";
  o << "}\n\n";

  // Jump arrows in the margin (Phase 8).
  o << "TextArrows\n{\n";
  o << "    qproperty-jump-in-function          : ${doki_info};\n";
  o << "    qproperty-jump-external-to-function : ${doki_eaccent};\n";
  o << "    qproperty-jump-under-cursor         : ${doki_fg};\n";
  o << "    qproperty-jump-target               : ${doki_eaccent};\n";
  o << "    qproperty-register-target           : ${doki_register};\n";
  o << "    qproperty-bpt-possible              : ${doki_warning};\n";
  o << "}\n\n";

  // CPU register pane (Phase 8).
  o << "TCpuRegs\n{\n";
  o << "    background-color                  : " << to_css_hex(pal.secondary_bg) << ";\n";
  o << "    qproperty-register-defined        : ${doki_fg};\n";
  o << "    qproperty-register-changed        : ${doki_eaccent};\n";
  o << "    qproperty-register-edited         : ${doki_name};\n";
  o << "    qproperty-register-unavailable    : ${doki_unused};\n";
  o << "}\n\n";

  o << "TCpuRegs[os-dark-theme=\"true\"]\n{\n";
  o << "    background-color                  : ${doki_surface};\n";
  o << "    qproperty-register-changed        : ${doki_eaccent};\n";
  o << "}\n\n";

  o << "TCpuRegs ui_label_t, TCpuRegs ui_label_t[os-dark-theme=\"true\"]\n{\n";
  o << "    color: " << to_css_hex(pal.cpu_label) << ";\n";
  o << "}\n\n";

  o << "TCpuRegs QPushButton\n{\n    background-color: transparent;\n}\n\n";

  o << "chooser_widget_t, standalone_dirtree_widget_t\n{\n";
  o << "    qproperty-highlight-default      : " << to_css_rgba(pal.chooser_highlight) << ";\n";
  o << "    qproperty-highlight-selected     : " << to_css_hex(pal.chooser_highlight_selected) << ";\n";
  o << "    qproperty-cut-text-default       : " << to_css_hex(pal.chooser_cut) << ";\n";
  o << "    qproperty-cut-text-selected      : " << to_css_hex(pal.chooser_cut_selected) << ";\n";
  o << "}\n\n";

  o << "log_widget_t\n{\n";
  o << "    color: " << to_css_hex(pal.log_fg) << ";\n";
  o << "    background-color: " << to_css_hex(pal.log_bg) << ";\n";
  o << "}\n\n";

  // IDA 9.4 dark/theme.css defines xref_tree_t with qproperty-tree-background-color
// and qproperty-search-match-color (not qproperty-tree-bg / qproperty-search-match).
// Cross-checked against D:\tools\IDA 9.4\themes\dark\theme.css lines 607-611.
o << "xref_tree_t\n{\n";
  o << "    qproperty-tree-background-color : " << to_css_hex(pal.xref_bg) << ";\n";
  o << "    qproperty-search-match-color    : " << rgba_alpha(pal.xref_search, 0.80) << ";\n";
  o << "}\n\n";

  o << "JumpAnywhereDialog\n{\n";
  o << "    background-color: " << to_css_hex(pal.jump_bg) << ";\n";
  o << "}\n\n";
  o << "JumpAnywhereDialog QListView::item:selected\n{\n";
  o << "    background: " << to_css_hex(pal.jump_selected_bg) << ";\n";
  o << "    color: " << to_css_hex(pal.jump_selected_fg) << ";\n";
  o << "}\n\n";
  o << "JumpAnywhereDialog QToolButton:hover\n{\n    background: " << to_css_rgba(pal.jump_button_hover) << ";\n}\n\n";
  o << "JumpAnywhereDialog QToolButton:checked\n{\n    background: " << to_css_rgba(pal.jump_button_checked) << ";\n}\n\n";

  o << "StyledQListView\n{\n    qproperty-query-annotation-color: ${doki_comment};\n}\n\n";

  o << "GraphMiniView\n{\n";
  o << "    qproperty-fog-color        : " << to_css_rgba(pal.graph_mini_fog) << ";\n";
  o << "    qproperty-crosshair-color  : " << to_css_hex(pal.graph_mini_crosshair) << ";\n";
  o << "}\n\n";

  o << "TChooser, diff_fringe_t\n{\n";
  o << "    qproperty-diff-inserted       : ${doki_diff_ins};\n";
  o << "    qproperty-diff-deleted        : ${doki_diff_del};\n";
  o << "    qproperty-diff-modified       : ${doki_diff_mod};\n";
  o << "    qproperty-diff-conflict       : ${doki_diff_conf};\n";
  o << "    qproperty-diff-pick-current   : " << to_css_rgba(pal.diff_pick) << ";\n";
  o << "    qproperty-diff-leave-current  : " << to_css_rgba(pal.diff_leave) << ";\n";
  o << "    qproperty-diff-boundary       : " << to_css_hex(pal.diff_boundary) << ";\n";
  o << "}\n\n";

  o << "pathfinder_t\n{\n";
  o << "    qproperty-drag-color   : " << to_css_hex(pal.pathfinder_drag) << ";\n";
  o << "    qproperty-delete-color : " << to_css_hex(pal.pathfinder_delete) << ";\n";
  o << "}\n\n";

  // IDA 9.4 dark/theme.css defines xg_view_t with qproperty-edge-default,
// qproperty-edge-highlighted, qproperty-viewer-bg, qproperty-path-waypoint-border,
// and qproperty-path-spine-border (cross-checked against
// D:\tools\IDA 9.4\themes\dark\theme.css lines 650-663). Earlier IDA versions
// used qproperty-edge / qproperty-path-waypoint / qproperty-path-spine without
// the suffix; these names are wrong for 9.4 and would be silently ignored.
o << "xg_view_t, xg_view_t[os-dark-theme=\"true\"]\n{\n";
  o << "    background-color                : " << to_css_hex(pal.xg_view_bg) << ";\n";
  o << "    qproperty-node-bg-default       : " << to_css_hex(pal.xg_node_bg) << ";\n";
  o << "    qproperty-node-bg-import        : " << to_css_hex(pal.xg_node_import) << ";\n";
  o << "    qproperty-node-bg-code          : " << to_css_hex(pal.xg_node_code) << ";\n";
  o << "    qproperty-node-bg-data          : " << to_css_hex(pal.xg_node_data) << ";\n";
  o << "    qproperty-node-border           : " << to_css_hex(pal.xg_node_border) << ";\n";
  o << "    qproperty-node-text             : " << to_css_hex(pal.xg_node_text) << ";\n";
  o << "    qproperty-edge-default          : " << to_css_hex(pal.xg_edge) << ";\n";
  o << "    qproperty-edge-highlighted      : " << to_css_hex(pal.editor_accent) << ";\n";
  o << "    qproperty-viewer-bg             : " << to_css_hex(pal.xg_view_bg) << ";\n";
  o << "    qproperty-path-waypoint-border  : " << to_css_hex(pal.xg_path) << ";\n";
  o << "    qproperty-path-spine-border     : " << to_css_hex(pal.xg_path) << ";\n";
  o << "}\n\n";

  o << "xref_graph_widget_t QWidget, xref_graph_widget_t[os-dark-theme=\"true\"] QWidget\n{\n";
  o << "    background-color: " << to_css_hex(pal.xg_view_bg) << ";\n";
  o << "}\n\n";

  // Qt chrome overrides (Phase 9). The base "dark" theme already styles
  // these; we retint with doki chrome colors.
  o << "QToolBar, QMenuBar, QMenuBar::item\n{\n";
  o << "    background-color: ${doki_chrome_hdr};\n";
  o << "    color: ${doki_fg};\n";
  o << "}\n\n";

  o << "QGroupBox\n{\n";
  o << "    background-color: ${doki_chrome_cntr};\n";
  o << "    color: ${doki_fg};\n";
  o << "}\n\n";

  o << "QToolTip, QTipLabel\n{\n";
  o << "    background-color: ${doki_chrome_cmp};\n";
  o << "    color: ${doki_fg};\n";
  o << "    border: 1px solid ${doki_border};\n";
  o << "}\n";

  return o.str();
}

} // namespace doki
