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
  Rgba window_bg;     // overall window background
  Rgba base_bg;       // editor / listing background
  Rgba secondary_bg;  // panels, headers, toolbars
  Rgba text;          // primary foreground text
  Rgba selection_bg;  // selection background
  Rgba selection_fg;  // selection foreground
  Rgba accent;        // accent / highlight color
  Rgba button_bg;     // button background
  Rgba button_fg;     // button text
  Rgba border;        // borders / separators
  Rgba line_number;   // line numbers / gutter
  Rgba caret_row;     // current line highlight
  Rgba disabled;      // disabled / muted text

  // --- Disassembly listing roles ---
  Rgba lst_bg;        // listing background
  Rgba lst_text;      // default instruction text
  Rgba lst_comment;   // comments
  Rgba lst_keyword;   // mnemonics / keywords
  Rgba lst_string;    // string literals
  Rgba lst_number;    // numeric constants
  Rgba lst_name;      // names / identifiers
  Rgba lst_register;  // registers

  // --- Navigation band ---
  Rgba nav_start;     // gradient start
  Rgba nav_stop;      // gradient stop
  Rgba nav_highlight; // highlighted ranges
};

// Build a fully-populated palette from a definition. Every role is guaranteed
// to be set: missing keys fall back to related keys, then to dark/light
// defaults, so no role is ever left empty.
IdaPalette map_theme(const DokiThemeDefinition &def);

} // namespace doki
