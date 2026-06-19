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
  o << "@def doki_search_bg    " << to_css_hex(pal.search_bg)         << ";\n";
  o << "@def doki_ident_hi     " << to_css_rgba(pal.ident_highlight)  << ";\n";
  o << "@def doki_bpt_active   " << to_css_rgba(pal.bp_active)        << ";\n";
  o << "@def doki_bpt_inactive " << to_css_rgba(pal.bp_inactive)      << ";\n";
  o << "@def doki_border       " << to_css_hex(pal.border)            << ";\n";
  o << "@def doki_chrome_hdr   " << to_css_hex(pal.chrome_header)     << ";\n";
  o << "@def doki_chrome_cntr  " << to_css_hex(pal.chrome_contrast)   << ";\n";
  o << "@def doki_chrome_cmp   " << to_css_hex(pal.chrome_completion_bg) << ";\n\n";

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
  o << "    qproperty-line-fg-keyword            : ${doki_keyword};\n";
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
  o << "    qproperty-line-fg-libfunc            : ${doki_name};\n";
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
  o << "    qproperty-line-fg-punctuation        : ${doki_fg};\n";
  o << "    qproperty-line-fg-alt-opnd           : ${doki_fg};\n";
  o << "    qproperty-line-fg-void-opnd          : ${doki_fg};\n";
  o << "    qproperty-line-fg-error              : ${doki_eaccent};\n";

  // ── selection (honor the AA byte -> translucent) ──
  o << "    qproperty-line-bg-selected           : ${doki_sel_bg_alpha};\n";

  // ── highlight backgrounds (8 user highlight slots) ──
  // Theme slots 1 and 2; slots 3-8 keep base defaults (would otherwise
  // inherit the stock yellow, which clashes with dark themes).
  o << "    qproperty-line-bg-highlight          : ${doki_search_bg};\n";
  o << "    qproperty-line-bg-highlight-2        : ${doki_ident_hi};\n";

  // ── current-line overlay (caretRow, subtle) ──
  o << "    qproperty-line-bgovl-current-line    : " << rgba_alpha(pal.caret_row, 0.35) << ";\n";

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
  o << "    qproperty-graph-node-title-normal    : ${doki_fg};\n";
  o << "    qproperty-graph-node-title-selected  : ${doki_fg};\n";
  o << "    qproperty-graph-node-title-current   : ${doki_eaccent};\n";
  o << "    qproperty-graph-node-shadow          : rgba(0,0,0,0.4);\n";
  o << "    qproperty-graph-edge-normal          : ${doki_eaccent};\n";
  o << "    qproperty-graph-edge-yes             : ${doki_eaccent};\n";
  o << "    qproperty-graph-edge-selected        : ${doki_name};\n";

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
  o << "    qproperty-function     : ${doki_name};\n";
  o << "    qproperty-lib-function : ${doki_info};\n";
  o << "    qproperty-code         : ${doki_keyword};\n";
  o << "    qproperty-data         : ${doki_string};\n";
  o << "    qproperty-undefined    : ${doki_unused};\n";
  o << "    qproperty-extern       : ${doki_register};\n";
  o << "    qproperty-cursor       : ${doki_eaccent};\n";
  o << "    qproperty-gap          : ${doki_bg};\n";
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
  o << "    qproperty-match-line-bg   : " << rgba_alpha(pal.search_bg, 0.25) << ";\n";
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
  o << "}\n\n";

  // CPU register pane (Phase 8).
  o << "TCpuRegs\n{\n";
  o << "    background-color                  : " << to_css_hex(pal.secondary_bg) << ";\n";
  o << "    qproperty-register-defined        : ${doki_fg};\n";
  o << "    qproperty-register-changed        : ${doki_eaccent};\n";
  o << "    qproperty-register-edited         : ${doki_name};\n";
  o << "    qproperty-register-unavailable    : ${doki_unused};\n";
  o << "}\n\n";

  // Qt chrome overrides (Phase 9). The base "dark" theme already styles
  // these; we retint with doki chrome colors.
  o << "QToolBar, QMenuBar\n{\n";
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
