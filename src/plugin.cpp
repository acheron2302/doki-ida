//----------------------------------------------------------------------------
// Doki Theme for IDA Pro - plugin entry point and controller.
//
// The plugin context owns the theme registry and the runtime applier, exposes
// the menu actions (IDokiActions), and routes run().
//----------------------------------------------------------------------------
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

#include "doki/log.h"
#include "doki/registry.h"
#include "doki/paths.h"
#include "doki/selftest.h"
#include "doki/installer.h"
#include "doki/apply.h"
#include "doki/ui.h"
#include "doki/config.h"

// Single definition of the verbose-logging flag declared in log.h.
bool doki::g_verbose = false;

//----------------------------------------------------------------------------
// Plugin context object. One instance per IDA session (PLUGIN_MULTI).
//----------------------------------------------------------------------------
struct doki_plugin_t : public plugmod_t, public doki::IDokiActions
{
  doki_plugin_t()
  {
    m_registry.load_dir(doki::definitions_dir());
    m_cfg = doki::load_config();

    // Capture the user's pre-doki theme once, so "Restore default" is faithful.
    std::string active = doki::current_theme();
    if ( m_cfg.original_theme.empty()
      && active.compare(0, 5, "doki-") != 0 )
    {
      m_cfg.original_theme = active.empty() ? "default" : active;
      doki::save_config(m_cfg);
    }

    doki::register_actions(this, this);
    doki::msg_log("loaded (v%s, SDK %d) - %u theme(s)\n",
                  DOKI_VERSION, IDA_SDK_VERSION, (uint)m_registry.size());

    // Re-apply the last selected character automatically.
    if ( !m_cfg.selected_id.empty() )
    {
      const doki::DokiThemeDefinition *t = m_registry.get(m_cfg.selected_id);
      if ( t != nullptr )
      {
        m_applier.apply(*t, /*activate=*/true,
                        m_cfg.sticker_enabled, m_cfg.wallpaper_enabled);
        doki::msg_log("restored last character: %s\n", t->displayName.c_str());
      }
    }
  }

  virtual ~doki_plugin_t()
  {
    // Cross-cutting invariant: leave IDA exactly as we found it.
    doki::unregister_actions();
    m_applier.shutdown();            // restore the nav colorizer
    doki::msg_log("unloaded\n");
  }

  //--- IDokiActions ---------------------------------------------------------
  virtual void open_picker() override
  {
    ssize_t i = doki::choose_theme(m_registry);
    if ( i >= 0 )
      apply_index((size_t)i);
  }

  virtual void apply_random() override
  {
    if ( m_registry.empty() )
      return;
    m_rng = m_rng * 1664525u + 1013904223u;       // LCG
    apply_index(m_rng % m_registry.size());
  }

  virtual void apply_next() override
  {
    if ( m_registry.empty() )
      return;
    apply_index((m_current + 1) % m_registry.size());
  }

  virtual void toggle_sticker() override
  {
    m_cfg.sticker_enabled = !m_cfg.sticker_enabled;
    doki::save_config(m_cfg);
    reapply_current();
    doki::msg_log("sticker %s\n", m_cfg.sticker_enabled ? "enabled" : "disabled");
  }

  virtual void toggle_wallpaper() override
  {
    m_cfg.wallpaper_enabled = !m_cfg.wallpaper_enabled;
    doki::save_config(m_cfg);
    reapply_current();
    doki::msg_log("wallpaper %s\n", m_cfg.wallpaper_enabled ? "enabled" : "disabled");
  }

  virtual void restore_default() override
  {
    m_applier.shutdown();                          // drop our nav colorizer
    const std::string orig = m_cfg.original_theme.empty()
                           ? std::string("default") : m_cfg.original_theme;
    doki::activate_theme(orig);
    m_cfg.selected_id.clear();
    doki::save_config(m_cfg);
    doki::msg_log("restored '%s' (restart to fully apply).\n", orig.c_str());
  }

  //--- run ------------------------------------------------------------------
  virtual bool idaapi run(size_t arg) override
  {
    switch ( arg )
    {
      case 1: return doki::run_selftest();         // dev: self-test
      case 2: return install_all();                // dev: install all
      case 3: apply_random(); return true;         // dev: apply one live
      default: open_picker(); return true;         // normal: the picker
    }
  }

private:
  void apply_index(size_t i)
  {
    if ( i >= m_registry.size() )
      return;
    m_current = i;
    const doki::DokiThemeDefinition &t = m_registry.themes()[i];
    m_applier.apply(t, /*activate=*/true,
                    m_cfg.sticker_enabled, m_cfg.wallpaper_enabled);
    m_cfg.selected_id = t.id;
    doki::save_config(m_cfg);
  }

  // Re-apply the currently selected theme (used by the sticker/wallpaper toggles).
  void reapply_current()
  {
    const doki::DokiThemeDefinition *t = m_registry.get(m_cfg.selected_id);
    if ( t != nullptr )
      m_applier.apply(*t, /*activate=*/true,
                      m_cfg.sticker_enabled, m_cfg.wallpaper_enabled);
  }

  bool install_all()
  {
    for ( const auto &t : m_registry.themes() )
      doki::install_theme(t, /*activate=*/false);
    doki::msg_log("installed %u theme(s).\n", (uint)m_registry.size());
    return true;
  }

  doki::ThemeRegistry m_registry;
  doki::ThemeApplier  m_applier;
  doki::DokiConfig    m_cfg;
  size_t              m_current = 0;
  uint32              m_rng = 0x9e3779b9u;
};

//----------------------------------------------------------------------------
static plugmod_t *idaapi init()
{
  return new doki_plugin_t;
}

//----------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_MULTI,                 // multi-instance, modern lifecycle
  init,                         // init -> returns the context object
  nullptr,                      // term  (unused with PLUGIN_MULTI)
  nullptr,                      // run   (handled by plugmod_t::run)
  "Doki Theme - anime character themes (color palettes + stickers) for IDA",
  "Doki Theme for IDA\n"
  "Brings the doki-theme experience to IDA Pro.",
  "Doki Theme",                 // name shown in Edit > Plugins
  nullptr                       // no global hotkey (picker is menu-driven)
};
