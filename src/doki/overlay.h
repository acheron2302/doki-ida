//----------------------------------------------------------------------------
// Doki Theme - transparent Qt6 overlay that paints the character sticker on
// top of an IDA viewer widget. We use a plain QWidget subclass (no Q_OBJECT)
// to avoid pulling moc/AUTOMOC into the build. IDA's Qt is namespaced (QT);
// we rely on the build's `QT_NAMESPACE=QT` define matching the SDK sample.
//----------------------------------------------------------------------------
#pragma once

#include <pro.h>
#include <kernwin.hpp>   // TWidget, HT_UI, is_idaq, hook_event_listener
#include <idp.hpp>       // event_listener_t (defined here, not in kernwin)

#include <QtWidgets/QWidget>
#include <QEvent>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QString>

#include <string>

namespace doki
{

// Transparent child widget. Lives on top of a target QWidget (the IDA view
// or pseudocode view), fills the target rect, and paints the sticker pixmap
// at the configured anchor. Mouse/keyboard input pass through. Visual alpha
// comes from the upstream sticker PNG itself; we paint at full painter
// opacity to honor the baked alpha of the official Doki assets.
class DokiStickerOverlayWidget : public QWidget
{
public:
  explicit DokiStickerOverlayWidget(QWidget *parent = nullptr);

  // Configure content. Pass an empty path to clear.
  void set_sticker(const QString &png_path,
                   const std::string &anchor);

  // Re-attach to a different target widget. Installs/removes an event filter
  // on the previous and new target. Re-parents this widget under the new
  // target and resizes to cover it.
  void set_target(QWidget *target);

  // Recompute our geometry to cover the current target. Safe to call any
  // time; the parent change is what actually rebinds things.
  void update_geometry();

  // Show/hide with the same visibility as the target.
  void sync_visibility();

  // Clear sticker pixmap/path and hide without changing the current target.
  void clear_sticker();

  // Clear target, remove event filter, hide. Used by manager on shutdown.
  void clear_target();

protected:
  // QWidget overrides.
  void paintEvent(QPaintEvent *) override;

  // QObject event filter for target's Resize/Move/Show/Hide/ParentChange.
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  QPixmap  m_pixmap;
  QString  m_pixmap_path;        // for diagnostics + reload
  std::string m_anchor = "bottom"; // center|left|right|top|bottom
  QPointer<QWidget> m_target;
};

// Manager that owns the single overlay widget and the HT_UI hook. One
// instance per ThemeApplier. Activates on update(), tears down on shutdown().
// All methods are safe to call multiple times.
class DokiOverlayManager : public event_listener_t
{
public:
  DokiOverlayManager();
  ~DokiOverlayManager() override;

  // Bring the overlay up to date. If `with_sticker` is false or
  // `sticker_installed` is false, the overlay is hidden. Idempotent.
  void update(const std::string &installed_theme_name,
              const std::string &sticker_file,
              const std::string &sticker_anchor,
              bool with_sticker,
              bool sticker_installed);

  // Detach and hide overlay; unhook HT_UI. Safe to call repeatedly.
  void shutdown();

  // HT_UI dispatch.
  ssize_t idaapi on_event(ssize_t code, va_list va) override;

private:
  // Resolve the target widget for overlay attachment. Tries active viewer,
  // then active widget, then a class-name search of the active widget's
  // children. Returns nullptr if no suitable target.
  QWidget *choose_target_widget();

  // Hook the active viewer/widget and place the overlay on it.
  void attach_to_current();

  // Disable overlay state until the next successful update(). Clears the
  // pixmap and target event filter so window events cannot resurrect it.
  void disable_overlay();

  // Compute installed sticker PNG path. Returns empty if not found.
  QString resolve_sticker_path(const std::string &installed_theme_name,
                                const std::string &sticker_file) const;

  bool m_hooked = false;
  bool m_enabled = false;            // true once a sticker is shown
  QString m_current_pixmap_path;     // last loaded path
  std::string m_current_anchor;
  std::string m_current_theme_name;  // for diagnostics
  std::string m_current_sticker_file; // for diagnostics

  QPointer<DokiStickerOverlayWidget> m_overlay;
};

} // namespace doki
