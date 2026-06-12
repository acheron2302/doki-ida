//----------------------------------------------------------------------------
// Doki Theme - theme.css generation.
//
// Produces an IDA theme stylesheet that imports a known-good base theme
// ("dark" or "default") and overrides the disassembly listing (CustomIDAMemo),
// navigation band (navband_t) and selection colors from the doki palette.
// Importing a base keeps all chrome widgets working; we only retint content.
//----------------------------------------------------------------------------
#pragma once

#include <string>

#include "doki/theme.h"
#include "doki/palette.h"

namespace doki
{

struct CssOptions
{
  // Emit a sticker as a fixed background-image on the listing. Off by default:
  // the translucent sticker is drawn by the Qt overlay (Phase 7); CSS has no
  // per-image opacity. Kept for environments without live Qt.
  bool include_sticker = false;
  std::string sticker_file;    // file name resolved via $RELPATH (theme folder)
  std::string sticker_anchor;  // "center" | "right" | "left" | ...

  // Optional full-listing wallpaper, painted on IDAViewHost behind everything.
  // When set, the sticker is layered on top (transparent memo background).
  bool include_wallpaper = false;
  std::string wallpaper_file;  // file name resolved via $RELPATH (theme folder)
};

// Generate a complete, self-contained theme.css body.
std::string generate_theme_css(const DokiThemeDefinition &def,
                               const IdaPalette &pal,
                               const CssOptions &opt);

} // namespace doki
