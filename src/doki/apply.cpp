//----------------------------------------------------------------------------
// Doki Theme - runtime theme application implementation.
//----------------------------------------------------------------------------
#include "doki/apply.h"
#include "doki/installer.h"
#include "doki/log.h"

#include <ida.hpp>
#include <kernwin.hpp>

namespace doki
{

// C-style trampoline matching nav_colorizer_t; forwards to the applier.
static uint32 idaapi doki_nav_colorizer(ea_t ea, asize_t nbytes, void *ud)
{
  return ((ThemeApplier *)ud)->colorize(ea, nbytes);
}

static uint8_t blend8(uint8_t a, uint8_t b, double t)
{
  double v = a + (b - a) * t;
  if ( v < 0 ) v = 0;
  if ( v > 255 ) v = 255;
  return (uint8_t)(v + 0.5);
}

uint32_t ThemeApplier::colorize(ea_t ea, asize_t /*nbytes*/)
{
  // Linear gradient nav_start -> nav_stop across the database address range.
  double t = 0.0;
  if ( m_max_ea > m_min_ea && ea >= m_min_ea && ea <= m_max_ea )
    t = double(ea - m_min_ea) / double(m_max_ea - m_min_ea);

  Rgba c;
  c.r = blend8(m_palette.nav_start.r, m_palette.nav_stop.r, t);
  c.g = blend8(m_palette.nav_start.g, m_palette.nav_stop.g, t);
  c.b = blend8(m_palette.nav_start.b, m_palette.nav_stop.b, t);
  return to_bgr24(c);
}

void ThemeApplier::ensure_colorizer_installed()
{
  if ( m_colorizer_installed )
    return;
  nav_colorizer_t *prev_func = nullptr;
  void *prev_ud = nullptr;
  set_nav_colorizer(&prev_func, &prev_ud, doki_nav_colorizer, this);
  m_prev_func = (void *)prev_func;
  m_prev_ud = prev_ud;
  m_colorizer_installed = true;
}

bool ThemeApplier::apply(const DokiThemeDefinition &def, bool activate,
                         bool with_sticker, bool with_wallpaper)
{
  InstallResult res = install_theme(def, activate, with_sticker, with_wallpaper);
  if ( !res.ok )
    return false;

  m_palette = map_theme(def);
  m_has_palette = true;
  m_min_ea = inf_get_min_ea();
  m_max_ea = inf_get_max_ea();

  ensure_colorizer_installed();
  refresh_navband(true);
  refresh_idaview_anyway();

  doki::msg_log("applied '%s'. Nav band updated live; restart IDA (or "
                "reselect in Options > Colors) to apply the full theme.\n",
                def.displayName.c_str());
  return true;
}

void ThemeApplier::shutdown()
{
  if ( !m_colorizer_installed )
    return;
  // Restore whatever colorizer was active before us.
  nav_colorizer_t *cur_func = nullptr;
  void *cur_ud = nullptr;
  set_nav_colorizer(&cur_func, &cur_ud,
                    (nav_colorizer_t *)m_prev_func, m_prev_ud);
  m_colorizer_installed = false;
  refresh_navband(true);
}

} // namespace doki
