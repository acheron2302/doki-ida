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
  virtual void restore_default() = 0;
  virtual ~IDokiActions() {}
};

// Register the doki actions and attach them under the Options menu.
// 'plugmod_owner' is the owning plugmod_t (for ACTION_DESC_LITERAL_PLUGMOD).
void register_actions(IDokiActions *actions, void *plugmod_owner);

// Detach + unregister the doki actions (safe to call if not registered).
void unregister_actions();

// Show the modal character chooser. Returns the chosen theme index in 'reg',
// or -1 if cancelled.
ssize_t choose_theme(const ThemeRegistry &reg);

} // namespace doki
