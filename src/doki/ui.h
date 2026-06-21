//----------------------------------------------------------------------------
// Doki Theme - menu actions + character chooser (pure SDK UI, no Qt).
//----------------------------------------------------------------------------
#pragma once

#include <pro.h>   // ssize_t and SDK base types

namespace doki
{

class ThemeRegistry;

// Implemented by the plugin context; the menu action handlers call back here.
struct IDokiActions
{
  virtual void open_picker() = 0;
  virtual void apply_random() = 0;
  virtual void apply_next() = 0;
  virtual void toggle_sticker() = 0;
  virtual void toggle_wallpaper() = 0;
  virtual void toggle_live_nav_colorizer() = 0;
  virtual void restore_default() = 0;

  // State accessors used to set the menu checkmark at registration time and
  // after every toggle. Plugin overrides return the persisted config flags.
  virtual bool is_sticker_enabled()                 { return true;  }
  virtual bool is_wallpaper_enabled()               { return true;  }
  virtual bool is_live_nav_colorizer_enabled()      { return false; }

  virtual ~IDokiActions() {}
};

// Register the doki actions and attach them under the Options menu.
// 'plugmod_owner' is the owning plugmod_t (for ACTION_DESC_LITERAL_PLUGMOD).
// Initial checkmark state for each toggle action is read from 'actions'
// via the is_*_enabled() accessors above.
void register_actions(IDokiActions *actions, void *plugmod_owner);

// Detach + unregister the doki actions (safe to call if not registered).
void unregister_actions();

// Refresh the menu checkmark of a registered action. 'name' is one of the
// DOKI_ACT_* constants. No-op if the action has not been registered.
void sync_action_checked(const char *name, bool checked);

// Show the modal character chooser. Returns the chosen theme index in 'reg',
// or -1 if cancelled.
ssize_t choose_theme(const ThemeRegistry &reg);

} // namespace doki
