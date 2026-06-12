//----------------------------------------------------------------------------
// Doki Theme - UI implementation (chooser + actions).
//----------------------------------------------------------------------------
#include "doki/ui.h"
#include "doki/registry.h"
#include "doki/log.h"

#include <pro.h>
#include <kernwin.hpp>

namespace doki
{

//----------------------------------------------------------------------------
// Character chooser: Character | Series | Mode.
//----------------------------------------------------------------------------
struct theme_chooser_t : public chooser_t
{
  const ThemeRegistry &reg;

  theme_chooser_t(const ThemeRegistry &reg_)
    : chooser_t(CH_MODAL | CH_KEEP, qnumber(s_widths), s_widths, s_header,
                "Doki Theme: pick a character"),
      reg(reg_) {}

  virtual size_t idaapi get_count() const override { return reg.size(); }

  virtual void idaapi get_row(
        qstrvec_t *out,
        int *out_icon,
        chooser_item_attrs_t *,
        size_t n) const override
  {
    out->resize(3);
    if ( n < reg.size() )
    {
      const DokiThemeDefinition &t = reg.themes()[n];
      (*out)[0] = t.displayName.c_str();
      (*out)[1] = t.group.c_str();
      (*out)[2] = t.dark ? "dark" : "light";
    }
    *out_icon = -1;
  }

  // enter() is unused here: we drive the chooser modally via choose() and read
  // the returned index, so selection handling lives in choose_theme().

  static const int s_widths[3];
  static const char *const s_header[3];
};

const int theme_chooser_t::s_widths[3] = { 24, 28, 6 };
const char *const theme_chooser_t::s_header[3] = { "Character", "Series", "Mode" };

ssize_t choose_theme(const ThemeRegistry &reg)
{
  if ( reg.empty() )
  {
    info("Doki Theme: no character themes are installed.");
    return -1;
  }
  theme_chooser_t *ch = new theme_chooser_t(reg);
  ssize_t sel = ch->choose(); // modal; returns index or a negative sentinel
  return sel >= 0 ? sel : -1;
}

//----------------------------------------------------------------------------
// Menu actions.
//----------------------------------------------------------------------------
static IDokiActions *g_actions = nullptr;

#define DOKI_ACT_PICK    "doki:pick"
#define DOKI_ACT_RANDOM  "doki:random"
#define DOKI_ACT_NEXT    "doki:next"
#define DOKI_ACT_STICKER "doki:toggle_sticker"
#define DOKI_ACT_WALL    "doki:toggle_wallpaper"
#define DOKI_ACT_RESTORE "doki:restore"

struct base_handler_t : public action_handler_t
{
  virtual action_state_t idaapi update(action_update_ctx_t *) override
  {
    return AST_ENABLE_ALWAYS;
  }
};

struct pick_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->open_picker(); return 1; }
};
struct random_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->apply_random(); return 1; }
};
struct next_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->apply_next(); return 1; }
};
struct sticker_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->toggle_sticker(); return 1; }
};
struct wallpaper_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->toggle_wallpaper(); return 1; }
};
struct restore_handler_t : public base_handler_t
{
  virtual int idaapi activate(action_activation_ctx_t *) override
  { if ( g_actions ) g_actions->restore_default(); return 1; }
};

static pick_handler_t      g_pick;
static random_handler_t    g_random;
static next_handler_t      g_next;
static sticker_handler_t   g_sticker;
static wallpaper_handler_t g_wall;
static restore_handler_t   g_restore;

struct act_def_t { const char *name; const char *label; action_handler_t *h; };
static const act_def_t g_defs[] =
{
  { DOKI_ACT_PICK,    "Doki Theme: Pick character...", &g_pick    },
  { DOKI_ACT_RANDOM,  "Doki Theme: Random character",  &g_random  },
  { DOKI_ACT_NEXT,    "Doki Theme: Next character",    &g_next    },
  { DOKI_ACT_STICKER, "Doki Theme: Toggle sticker",    &g_sticker },
  { DOKI_ACT_WALL,    "Doki Theme: Toggle wallpaper",  &g_wall    },
  { DOKI_ACT_RESTORE, "Doki Theme: Restore default",   &g_restore },
};

void register_actions(IDokiActions *actions, void *plugmod_owner)
{
  g_actions = actions;
  for ( const auto &d : g_defs )
  {
    action_desc_t desc = ACTION_DESC_LITERAL_PLUGMOD(
        d.name, d.label, d.h, (plugmod_t *)plugmod_owner,
        nullptr, nullptr, -1);
    if ( !register_action(desc) )
      doki::msg_log("failed to register action %s\n", d.name);
    else
      attach_action_to_menu("Options/", d.name, SETMENU_APP);
  }
  doki::msg_log("menu actions registered under Options.\n");
}

void unregister_actions()
{
  for ( const auto &d : g_defs )
  {
    detach_action_from_menu("Options/", d.name);
    unregister_action(d.name);
  }
  g_actions = nullptr;
}

} // namespace doki
