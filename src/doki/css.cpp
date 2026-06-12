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
  const bool stick = opt.include_sticker && !opt.sticker_file.empty();
  const bool wall  = opt.include_wallpaper && !opt.wallpaper_file.empty();
  // Any image behind the listing requires transparent line/widget backgrounds.
  const bool transparent_bg = stick || wall;

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

  // Disassembly listing. When a sticker is shown, the per-line background must
  // be transparent so the image painted on the widget shows through; otherwise
  // it stays the opaque doki background. (See hex-rays.com/blog/ui-candy.)
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

  if ( stick )
  {
    // The sticker is painted on the memo. With a wallpaper behind it the memo
    // base must be transparent (so the wallpaper shows through); without one
    // the memo base is the opaque doki color.
    o << "\n    background            : "
      << (wall ? std::string("transparent") : to_css_hex(pal.lst_bg))
      << " url(\"$RELPATH/" << opt.sticker_file << "\");\n";
    o << "    background-repeat     : no-repeat;\n";
    o << "    background-position   : " << anchor_to_position(opt.sticker_anchor) << ";\n";
    o << "    background-attachment : fixed;\n";
  }
  else if ( wall )
  {
    // No sticker but a wallpaper: keep the memo transparent so it shows.
    o << "\n    background            : transparent;\n";
  }
  o << "}\n\n";

  if ( transparent_bg )
  {
    // Keep the fringe/arrows from painting over the image(s).
    o << "ecfringe_t  { background: transparent; }\n";
    o << "TextArrows  { background: transparent; }\n";
    if ( wall )
    {
      // The wallpaper lives on the view host, behind the (transparent) memo.
      o << "IDAViewHost\n{\n";
      o << "    background            : ${doki_bg} url(\"$RELPATH/"
        << opt.wallpaper_file << "\");\n";
      o << "    background-repeat     : no-repeat;\n";
      o << "    background-position   : center center;\n";
      o << "    background-attachment : fixed;\n";
      o << "}\n\n";
    }
    else
    {
      o << "IDAViewHost { background: ${doki_bg}; }\n\n";
    }
  }

  // Navigation band base color (per-range colors come from the colorizer).
  o << "navband_t\n{\n";
  o << "    background-color : ${doki_bg};\n";
  o << "}\n";

  return o.str();
}

} // namespace doki
