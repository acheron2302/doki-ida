//----------------------------------------------------------------------------
// Doki Theme - overlay implementation.
//
// All Qt-using code is guarded by is_idaq() so the plugin can still be
// loaded in tty_idalib contexts (where there is no Qt at all) without
// touching Qt symbols. In tty mode, the overlay is a no-op.
//----------------------------------------------------------------------------
#include "doki/overlay.h"

#include "doki/log.h"
#include "doki/paths.h"

#include <ida.hpp>
#include <kernwin.hpp>

#include <QtWidgets>
#include <QApplication>
#include <QFileInfo>
#include <QString>

#include <algorithm>
#include <cmath>

namespace doki
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{

// Map our anchor string to a placement flag.
enum class Anchor
{
  Center,
  Left,
  Right,
  Top,
  Bottom
};

Anchor parse_anchor(const std::string &s)
{
  if ( s == "center" ) return Anchor::Center;
  if ( s == "left" )   return Anchor::Left;
  if ( s == "right" )  return Anchor::Right;
  if ( s == "top" )    return Anchor::Top;
  if ( s == "bottom" ) return Anchor::Bottom;
  return Anchor::Right; // default: corner mascot
}

// Bound an integer to [lo, hi].
int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

// Compute the destination rect for the sticker inside `bounds` (the
// overlay's geometry). The sticker is painted at its NATIVE pixel size
// (no scaling) and positioned at the chosen anchor. The overlay widget
// itself is larger than the sticker; only the part of the overlay that
// covers the sticker draws anything because paintEvent is a no-op outside
// the destination rect.
QRect compute_dest_rect(const QRect &bounds,
                        const QSize &pix_size,
                        Anchor anchor,
                        int margin)
{
  if ( bounds.isEmpty() || pix_size.isEmpty() )
    return QRect();

  const int w = pix_size.width();
  const int h = pix_size.height();

  int x = 0, y = 0;
  switch ( anchor )
  {
    case Anchor::Center:
      x = bounds.x() + (bounds.width()  - w) / 2;
      y = bounds.y() + (bounds.height() - h) / 2;
      break;
    case Anchor::Left:
      x = bounds.x() + margin;
      y = bounds.y() + margin;
      break;
    case Anchor::Right:
      x = bounds.x() + bounds.width()  - w - margin;
      y = bounds.y() + margin;
      break;
    case Anchor::Top:
      x = bounds.x() + (bounds.width()  - w) / 2;
      y = bounds.y() + margin;
      break;
    case Anchor::Bottom:
      x = bounds.x() + bounds.width() - w - margin;
      y = bounds.y() + bounds.height() - h - margin;
      break;
  }
  return QRect(x, y, w, h);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DokiStickerOverlayWidget
// ---------------------------------------------------------------------------

DokiStickerOverlayWidget::DokiStickerOverlayWidget(QWidget *parent)
  : QWidget(parent)
{
  // Transparent overlay: paint only the pixmap alpha, never block input.
  setAttribute(Qt::WA_TransparentForMouseEvents, true);
  setAttribute(Qt::WA_NoSystemBackground,       true);
  setAttribute(Qt::WA_TranslucentBackground,    true);
  setAutoFillBackground(false);
  setFocusPolicy(Qt::NoFocus);
  hide();
}

void DokiStickerOverlayWidget::set_sticker(const QString &png_path,
                                           const std::string &anchor,
                                           int opacity_percent)
{
  m_anchor = anchor;
  m_opacity = qreal(clampi(opacity_percent, 0, 100)) / qreal(100.0);
  m_pixmap_path = png_path;

  if ( png_path.isEmpty() || !QFileInfo(png_path).isFile() )
  {
    m_pixmap = QPixmap();
    update();
    hide();
    return;
  }

  m_pixmap = QPixmap(png_path);
  if ( m_pixmap.isNull() )
  {
    doki::msg_log("  overlay: failed to load sticker '%s'\n",
                  png_path.toUtf8().constData());
    hide();
    return;
  }

  // First time we get a valid sticker: show ourselves.
  if ( m_target )
  {
    sync_visibility();
  }
  update();
}

void DokiStickerOverlayWidget::set_target(QWidget *target)
{
  if ( m_target == target )
  {
    update_geometry();
    return;
  }

  if ( m_target )
    m_target->removeEventFilter(this);

  m_target = target;
  if ( m_target )
  {
    setParent(m_target);
    m_target->installEventFilter(this);
  }
  else
  {
    setParent(nullptr);
  }
  update_geometry();
}

void DokiStickerOverlayWidget::update_geometry()
{
  if ( m_target )
  {
    // Cover the full target rect.
    setGeometry(m_target->rect());
    sync_visibility();
  }
  else
  {
    hide();
  }
}

void DokiStickerOverlayWidget::sync_visibility()
{
  const bool want = m_target != nullptr
                 && m_target->isVisible()
                 && !m_pixmap.isNull();
  if ( want )
  {
    if ( !isVisible() ) show();
    raise();
  }
  else
  {
    if ( isVisible() ) hide();
  }
}

void DokiStickerOverlayWidget::clear_target()
{
  if ( m_target )
  {
    m_target->removeEventFilter(this);
    m_target = nullptr;
  }
  setParent(nullptr);
  hide();
}

void DokiStickerOverlayWidget::paintEvent(QPaintEvent *)
{
  if ( m_pixmap.isNull() )
    return;

  QPainter p(this);
  if ( !p.isActive() )
    return;
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  p.setOpacity(m_opacity);

  const QRect bounds = rect();
  const QRect dst = compute_dest_rect(bounds, m_pixmap.size(),
                                      parse_anchor(m_anchor),
                                      /*margin=*/16);

  if ( !dst.isEmpty() )
    p.drawPixmap(dst, m_pixmap);
}

bool DokiStickerOverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
  if ( watched != m_target )
    return false;
  switch ( event->type() )
  {
    case QEvent::Resize:
    case QEvent::Move:
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::ParentChange:
    case QEvent::ZOrderChange:
      update_geometry();
      break;
    default:
      break;
  }
  return false;
}

// ---------------------------------------------------------------------------
// DokiOverlayManager
// ---------------------------------------------------------------------------

DokiOverlayManager::DokiOverlayManager()
{
  if ( !is_idaq() || QApplication::instance() == nullptr )
    return;

  // owner = this: the kernel will unhook the listener when the manager
  // (and therefore the plugin module) is unloaded. HKCB_GLOBAL is not
  // required: our lifetime matches the plugin lifetime.
  hook_event_listener(HT_UI, this, (const void *)this);
  m_hooked = true;
}

DokiOverlayManager::~DokiOverlayManager()
{
  shutdown();
  // event_listener_t unhooks itself in its dtor if still hooked; we keep
  // m_hooked as a flag to avoid double-unhook in shutdown().
}

void DokiOverlayManager::shutdown()
{
  if ( m_overlay )
  {
    m_overlay->clear_target();
    m_overlay = nullptr;
  }
  m_enabled = false;
  m_current_pixmap_path.clear();
  m_current_theme_name.clear();
  m_current_sticker_file.clear();
  // Listener unhook is handled by the IDA event_listener_t destructor when
  // the manager object itself is destroyed. We don't call
  // unhook_event_listener(HT_UI, this) here to avoid double-unhook.
}

QString DokiOverlayManager::resolve_sticker_path(
    const std::string &installed_theme_name,
    const std::string &sticker_file) const
{
  if ( sticker_file.empty() || installed_theme_name.empty() )
    return QString();
  // Primary: themes/<name>/<sticker>
  const std::string primary = path_join(
      path_join(themes_dir(), installed_theme_name), sticker_file);
  if ( QFileInfo(QString::fromStdString(primary)).isFile() )
    return QString::fromStdString(primary);
  // Fallback: assets/stickers/<sticker>
  const std::string fallback = path_join(
      path_join(assets_dir(), "stickers"), sticker_file);
  if ( QFileInfo(QString::fromStdString(fallback)).isFile() )
    return QString::fromStdString(fallback);
  return QString();
}

void DokiOverlayManager::update(const std::string &installed_theme_name,
                                const std::string &sticker_file,
                                const std::string &sticker_anchor,
                                int opacity_percent,
                                bool with_sticker,
                                bool sticker_installed)
{
  if ( !is_idaq() || QApplication::instance() == nullptr )
    return;

  m_current_theme_name   = installed_theme_name;
  m_current_sticker_file = sticker_file;

  if ( !with_sticker || !sticker_installed || sticker_file.empty() )
  {
    if ( m_overlay )
      m_overlay->hide();
    m_enabled = false;
    return;
  }

  const QString path = resolve_sticker_path(installed_theme_name, sticker_file);
  if ( path.isEmpty() )
  {
    doki::msg_log("  overlay: sticker '%s' not found in theme or assets\n",
                  sticker_file.c_str());
    if ( m_overlay )
      m_overlay->hide();
    m_enabled = false;
    return;
  }

  if ( !m_overlay )
    m_overlay = new DokiStickerOverlayWidget();

  m_overlay->set_sticker(path, sticker_anchor, opacity_percent);
  m_current_pixmap_path = path;
  m_current_anchor      = sticker_anchor;
  m_current_opacity     = opacity_percent;
  m_enabled = true;

  attach_to_current();
}

QWidget *DokiOverlayManager::choose_target_widget()
{
  // 1) Active viewer (disassembly / pseudocode / custom viewer).
  TWidget *viewer = get_current_viewer();
  if ( viewer != nullptr )
    return (QWidget *) viewer;

  // 2) Active widget (any focus widget).
  TWidget *cur = get_current_widget();
  if ( cur != nullptr )
    return (QWidget *) cur;

  return nullptr;
}

void DokiOverlayManager::attach_to_current()
{
  if ( !m_overlay || !m_enabled )
    return;

  QWidget *target = choose_target_widget();
  if ( target == nullptr )
  {
    // No active target yet; keep overlay hidden, will retry on next event.
    m_overlay->hide();
    return;
  }

  m_overlay->set_target(target);
}

ssize_t idaapi DokiOverlayManager::on_event(ssize_t code, va_list va)
{
  // We only care about a few HT_UI notifications.
  switch ( code )
  {
    case ui_current_widget_changed:
    {
      // The active widget changed; re-evaluate the target.
      attach_to_current();
      break;
    }
    case ui_widget_visible:
    {
      TWidget *w = va_arg(va, TWidget *);
      QWidget *qw = w ? (QWidget *)w : nullptr;
      // If a viewer-like widget just became visible, prefer it.
      if ( qw != nullptr && qw == (QWidget *)get_current_viewer() )
        attach_to_current();
      break;
    }
    case ui_widget_invisible:
    case ui_widget_closing:
    {
      // If our target went away, hide until a new active one shows up.
      if ( m_overlay && m_overlay->parentWidget() == nullptr )
        m_overlay->hide();
      break;
    }
    default:
      break;
  }
  return 0;
}

} // namespace doki
