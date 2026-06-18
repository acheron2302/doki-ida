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
  o << "@def doki_bg        " << to_css_hex(pal.lst_bg)       << ";\n";
  o << "@def doki_fg        " << to_css_hex(pal.lst_text)     << ";\n";
  o << "@def doki_comment   " << to_css_hex(pal.lst_comment)  << ";\n";
  o << "@def doki_keyword   " << to_css_hex(pal.lst_keyword)  << ";\n";
  o << "@def doki_string    " << to_css_hex(pal.lst_string)   << ";\n";
  o << "@def doki_number    " << to_css_hex(pal.lst_number)   << ";\n";
  o << "@def doki_name      " << to_css_hex(pal.lst_name)     << ";\n";
  o << "@def doki_register  " << to_css_hex(pal.lst_register) << ";\n";
  o << "@def doki_accent    " << to_css_hex(pal.accent)       << ";\n";
  o << "@def doki_sel_bg    " << to_css_hex(pal.selection_bg) << ";\n";
  o << "@def doki_caret_row " << rgba_alpha(pal.caret_row, 0.25) << ";\n\n";

  // Disassembly and pseudocode listings both use CustomIDAMemo. When a
  // wallpaper is layered behind the listing, the per-line background must
  // be transparent so the image shows through.
  // See https://hex-rays.com/blog/ui-candy for the official approach.
  o << "CustomIDAMemo\n{\n";
  if ( transparent_bg )
    o << "    qproperty-line-bg-default            : " << rgba_alpha(pal.lst_bg, 0.0) << ";\n";
  else
    o << "    qproperty-line-bg-default            : ${doki_bg};\n";
  o << "    qproperty-line-fg-default            : ${doki_fg};\n";
  o << "    qproperty-line-fg-insn               : ${doki_keyword};\n";
  o << "    qproperty-line-fg-keyword            : ${doki_keyword};\n";
  o << "    qproperty-line-fg-asm-directive      : ${doki_keyword};\n";
  o << "    qproperty-line-fg-register-name      : ${doki_register};\n";
  o << "    qproperty-line-fg-regular-comment    : ${doki_comment};\n";
  o << "    qproperty-line-fg-repeatable-comment : ${doki_comment};\n";
  o << "    qproperty-line-fg-automatic-comment  : ${doki_comment};\n";
  o << "    qproperty-line-fg-strlit-in-insn     : ${doki_string};\n";
  o << "    qproperty-line-fg-charlit-in-insn    : ${doki_string};\n";
  o << "    qproperty-line-fg-strlit-in-data     : ${doki_string};\n";
  o << "    qproperty-line-fg-charlit-in-data    : ${doki_string};\n";
  o << "    qproperty-line-fg-numlit-in-insn     : ${doki_number};\n";
  o << "    qproperty-line-fg-numlit-in-data     : ${doki_number};\n";
  o << "    qproperty-line-fg-regular-data-name  : ${doki_name};\n";
  o << "    qproperty-line-fg-dummy-data-name    : ${doki_name};\n";
  o << "    qproperty-line-fg-code-name          : ${doki_name};\n";
  o << "    qproperty-line-fg-demangled-name     : ${doki_name};\n";
  o << "    qproperty-line-fg-libfunc            : ${doki_name};\n";
  o << "    qproperty-line-fg-locvar             : ${doki_name};\n";
  o << "    qproperty-line-bg-selected           : ${doki_sel_bg};\n";
  o << "    qproperty-caret                      : ${doki_fg};\n";
  o << "    qproperty-line-bg-highlight          : ${doki_caret_row};\n";

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

      o << "text_area_t QWidget\n{\n";
      o << "    background-color: transparent;\n";
      o << "}\n\n";

      o << "text_area_t text_area_margin_widget_t\n{\n";
      o << "    background-color: transparent;\n";
      o << "}\n\n";
    }
  }

  // Navigation band base color (per-range colors come from the colorizer).
  o << "navband_t\n{\n";
  o << "    background-color : ${doki_bg};\n";
  o << "}\n\n";

  // Hex-Rays pseudocode / text-area colors. These selectors are inspired by
  // IDA's built-in dark theme and the public Monokai-Light-IDA theme; the
  // qproperty-* keys map to the fields exposed by text_area_t.
  o << "text_area_t\n{\n";
  o << "    qproperty-keyword1-fg     : ${doki_keyword};\n";
  o << "    qproperty-keyword2-fg     : ${doki_accent};\n";
  o << "    qproperty-keyword3-fg     : ${doki_number};\n";
  o << "    qproperty-string-fg       : ${doki_string};\n";
  o << "    qproperty-comment-fg      : ${doki_comment};\n";
  o << "    qproperty-preprocessor-fg : ${doki_keyword};\n";
  o << "    qproperty-number-fg       : ${doki_number};\n";
  o << "    qproperty-user1-fg        : ${doki_name};\n";
  o << "    qproperty-match-line-bg   : ${doki_caret_row};\n";
  o << "    qproperty-match-text-bg   : ${doki_sel_bg};\n";
  o << "}\n\n";

  o << "text_area_t text_area_margin_widget_t\n{\n";
  o << "    color                  : ${doki_comment};\n";
  o << "    qproperty-header-color : ${doki_fg};\n";
  o << "}\n\n";

  // Decompiler semantic colors exposed via syntax_color_t. These cover the
  // distinct categories Hex-Rays emits (function names, library funcs,
  // imports, data references, etc.).
  o << "syntax_color_t\n{\n";
  o << "    qproperty-function-name-color        : ${doki_name};\n";
  o << "    qproperty-library-function-color     : ${doki_name};\n";
  o << "    qproperty-imported-function-color    : ${doki_accent};\n";
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
  o << "    qproperty-code-reference-color       : ${doki_accent};\n";
  o << "    qproperty-data-reference-color       : ${doki_number};\n";
  o << "    qproperty-prefix-color               : ${doki_comment};\n";
  o << "}\n";

  return o.str();
}

} // namespace doki
