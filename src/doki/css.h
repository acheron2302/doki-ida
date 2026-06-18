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
  // Optional full-listing wallpaper, painted on CustomIDAMemo. When set, the
  // listing line background is forced transparent and the pseudocode text
  // area is also transparent so the wallpaper shows through both views.
  // The wallpaper is the upstream Doki PNG; the per-pixel alpha baked into
  // that file is the only transparency in play (no CSS opacity, no Qt layer).
  bool include_wallpaper = false;
  std::string wallpaper_file;  // file name resolved via $RELPATH (theme folder)
  std::string wallpaper_anchor; // "center" | "right" | "left" | "top" | "bottom"
};

// Generate a complete, self-contained theme.css body.
std::string generate_theme_css(const DokiThemeDefinition &def,
                               const IdaPalette &pal,
                               const CssOptions &opt);

} // namespace doki
