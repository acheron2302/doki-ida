//----------------------------------------------------------------------------
// Doki Theme - runtime theme application implementation.
//----------------------------------------------------------------------------
#include "doki/apply.h"
#include "doki/installer.h"
#include "doki/log.h"
#include "doki/overlay.h"

#include <ida.hpp>
#include <kernwin.hpp>

#include <cctype>

namespace doki
{

ThemeApplier::ThemeApplier()  = default;
ThemeApplier::~ThemeApplier() = default;

// Sticker anchor policy: pinned to the bottom-right corner regardless of what
// the theme's JSON says. The upstream Doki definitions still carry
// per-character anchors ("center" for Echidna, "right" for most others)
// inherited from their CSS-only days, but the IDA overlay uses a single
// placement -- the bottom-right corner of the active view. "bottom" maps to
// the bottom-right in overlay.cpp's compute_dest_rect.
constexpr const char *kOverlayStickerAnchor = "bottom";

// Diagnostic helper: classify the raw anchor from the theme definition so
// the apply logs make it clear that we deliberately override the theme's
// value. Returns the recognized anchor name in lower case (or "empty" /
// "unrecognized") for logging only; the actual value passed to the overlay
// is always kOverlayStickerAnchor.
static const char *classify_anchor_for_log(const std::string &raw_anchor)
{
  if ( raw_anchor.empty() )
    return "empty";
  std::string lowered;
  lowered.reserve(raw_anchor.size());
  for ( char c : raw_anchor )
    lowered.push_back(static_cast<char>(std::tolower(
        static_cast<unsigned char>(c))));
  static const char *kAccepted[] = {
      "center", "left", "right", "top", "bottom"
  };
  for ( const char *a : kAccepted )
    if ( lowered == a )
      return a;
  return "unrecognized";
}

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

void ThemeApplier::ensure_overlay_manager()
{
  if ( m_overlay )
    return;
  // The DokiOverlayManager constructor no-ops in tty mode and when Qt is not
  // available, so this is safe in all contexts.
  m_overlay = std::make_unique<DokiOverlayManager>();
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

  // Drive the Qt overlay from the same install result. The overlay is the
  // only sticker renderer in the Qt build; CSS no longer paints it.
  ensure_overlay_manager();
  if ( m_overlay )
  {
    // The overlay is pinned to the bottom-right corner of the active view
    // regardless of what the theme's JSON says. The raw anchor from the
    // definition is logged for diagnostics only.
    doki::msg_log("  apply: sticker anchor='%s' (forced='%s')\n",
                  classify_anchor_for_log(def.sticker.anchor),
                  kOverlayStickerAnchor);

    m_overlay->update(res.theme_name,
                      def.sticker.name,
                      kOverlayStickerAnchor,
                      with_sticker,
                      res.sticker_installed);
  }

  if ( activate )
  {
    doki::msg_log("applied '%s'. Nav band updated live; restart IDA (or "
                  "reselect in Options > Colors) to apply the full theme.\n",
                  def.displayName.c_str());
  }
  else
  {
    doki::msg_log("applied '%s'. Nav band updated live; IDA theme left unchanged.\n",
                  def.displayName.c_str());
  }
  return true;
}

void ThemeApplier::apply_live_only(const DokiThemeDefinition &def,
                                   bool with_sticker)
{
  m_palette = map_theme(def);
  m_has_palette = true;
  m_min_ea = inf_get_min_ea();
  m_max_ea = inf_get_max_ea();

  ensure_colorizer_installed();
  refresh_navband(true);
  refresh_idaview_anyway();

  // The sticker overlay reads from the already-installed theme folder,
  // so no network fetch is involved here.
  ensure_overlay_manager();
  if ( m_overlay )
  {
    // Same bottom-right pinning as apply(). See kOverlayStickerAnchor.
    doki::msg_log("  apply_live: sticker anchor='%s' (forced='%s')\n",
                  classify_anchor_for_log(def.sticker.anchor),
                  kOverlayStickerAnchor);
    m_overlay->update(theme_name_for(def),
                      def.sticker.name,
                      kOverlayStickerAnchor,
                      with_sticker,
                      /*sticker_installed=*/true);
  }
}

void ThemeApplier::shutdown()
{
  // Tear down the overlay first so we never leave a stuck Qt widget behind
  // even if the nav colorizer was never installed (e.g. tty_idalib).
  if ( m_overlay )
  {
    m_overlay->shutdown();
    m_overlay.reset();
  }

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
