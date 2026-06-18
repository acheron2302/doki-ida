//----------------------------------------------------------------------------
// Doki Theme - runtime theme application (Phases 7 + 8, merged: with the Qt
// overlay dropped, activation and live SDK coloring form one unit).
//
//  * install + activate the theme (Phase 6)               -> chrome/listing
//    take effect on the next IDA launch (CSS is loaded at startup).
//  * register a live navigation-band colorizer (Phase 8)  -> takes effect
//    immediately, painting the nav band with the doki gradient.
//  * update the Qt sticker overlay                        -> takes effect
//    immediately, painting the character artwork on top of the active view.
//
// The colorizer is always restored on teardown so unloading the plugin never
// leaves IDA in a broken state.
//----------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>

#include <pro.h>   // ea_t, asize_t

#include "doki/theme.h"
#include "doki/palette.h"

namespace doki
{

// Forward declaration to keep Qt out of this header.
class DokiOverlayManager;

class ThemeApplier
{
public:
  ThemeApplier();
  ~ThemeApplier();

  // Install (Phase 6) + activate + apply live nav colors. Returns false on
  // install failure. 'activate' writes the registry ThemeName;
  // 'with_sticker' controls whether the sticker is installed;
  // 'with_wallpaper' controls whether the full-listing wallpaper is
  // installed (and emitted in the generated CSS).
  bool apply(const DokiThemeDefinition &def,
             bool activate = true,
             bool with_sticker = true,
             bool with_wallpaper = true);

  // Live-only path used by auto-restore on plugin init. Re-applies the
  // nav colorizer and the sticker overlay without writing the theme
  // folder or fetching assets from the network. Safe to call before the
  // GUI is fully realized.
  void apply_live_only(const DokiThemeDefinition &def,
                       bool with_sticker = true);

  // Restore the previous nav colorizer (idempotent). Called from the plugin
  // destructor.
  void shutdown();

  bool has_palette() const { return m_has_palette; }
  const IdaPalette &palette() const { return m_palette; }

  // The nav colorizer callback (public so the C-style trampoline can reach it).
  uint32_t colorize(ea_t ea, asize_t nbytes);

private:
  void ensure_colorizer_installed();
  void ensure_overlay_manager();

  IdaPalette m_palette;
  bool m_has_palette = false;

  bool m_colorizer_installed = false;
  void *m_prev_func = nullptr;   // nav_colorizer_t* (opaque here)
  void *m_prev_ud = nullptr;

  ea_t m_min_ea = 0;
  ea_t m_max_ea = 0;

  // Lazily created on first apply() in a GUI build; destroyed in shutdown().
  std::unique_ptr<DokiOverlayManager> m_overlay;
};

} // namespace doki
