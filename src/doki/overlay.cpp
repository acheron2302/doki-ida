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
#include <QTimer>

#include <algorithm>
#include <functional>

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
                                           const std::string &anchor)
{
  m_anchor = anchor;
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
  // QPointer guard: m_target may have been destroyed between event filter
  // callbacks if IDA tore down the view underneath us.
  if ( m_target.isNull() )
  {
    m_target = nullptr;
    hide();
    return;
  }
  // Cover the full target rect.
  setGeometry(m_target->rect());
  raise();
  sync_visibility();
  update();
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

void DokiStickerOverlayWidget::clear_sticker()
{
  m_pixmap = QPixmap();
  m_pixmap_path.clear();
  update();
  hide();
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
  // Visual alpha is the upstream PNG's own baked alpha. We paint at full
  // painter opacity; the PNG's per-pixel alpha does the rest.
  p.setOpacity(1.0);

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
    case QEvent::ChildAdded:
      // IDA swaps inner view children (e.g. when switching between
      // disassembly and pseudocode). Defer the re-raise to the next event
      // loop tick so the new child is fully realized first.
      QTimer::singleShot(0, this, [this] { update_geometry(); });
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

void DokiOverlayManager::disable_overlay()
{
  m_enabled = false;
  m_current_pixmap_path.clear();
  m_current_anchor.clear();
  if ( m_overlay )
  {
    m_overlay->clear_sticker();
    m_overlay->clear_target();
  }
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
  // Fallback: assets/stickers/<sticker> (compatibility / offline use).
  const std::string fallback = path_join(
      path_join(assets_dir(), "stickers"), sticker_file);
  if ( QFileInfo(QString::fromStdString(fallback)).isFile() )
    return QString::fromStdString(fallback);
  return QString();
}

void DokiOverlayManager::update(const std::string &installed_theme_name,
                                const std::string &sticker_file,
                                const std::string &sticker_anchor,
                                bool with_sticker,
                                bool sticker_installed)
{
  if ( !is_idaq() || QApplication::instance() == nullptr )
    return;

  m_current_theme_name   = installed_theme_name;
  m_current_sticker_file = sticker_file;

  if ( !with_sticker || !sticker_installed || sticker_file.empty() )
  {
    disable_overlay();
    return;
  }

  const QString path = resolve_sticker_path(installed_theme_name, sticker_file);
  if ( path.isEmpty() )
  {
    doki::msg_log("  overlay: sticker '%s' not found in theme or assets\n",
                  sticker_file.c_str());
    disable_overlay();
    return;
  }

  if ( !m_overlay )
    m_overlay = new DokiStickerOverlayWidget();

  m_overlay->set_sticker(path, sticker_anchor);
  m_current_pixmap_path = path;
  m_current_anchor      = sticker_anchor;
  m_enabled = true;

  attach_to_current();
}

QWidget *DokiOverlayManager::choose_target_widget()
{
  // Helper: walk the descendants of `root` and return the first widget whose
  // class name or object name matches the IDA paint surface markers
  // ("CustomIDAMemo" for the listing, "text_area_t" for the Hex-Rays
  // pseudocode widget). Depth-first, so the inner-most match wins.
  auto find_paint_surface = [](QWidget *root) -> QWidget * {
    if ( root == nullptr )
      return nullptr;
    QWidget *match = nullptr;
    std::function<void(QWidget *)> walk = [&](QWidget *w) {
      if ( w == nullptr )
        return;
      const QMetaObject *mo = w->metaObject();
      const QString cname = mo ? QString::fromLatin1(mo->className())
                               : QString::fromLatin1(w->metaObject()->className());
      const QString oname = w->objectName();
      const bool is_memo = cname.contains("CustomIDAMemo")
                        || oname.contains("CustomIDAMemo");
      const bool is_text = cname.contains("text_area_t")
                        || oname.contains("text_area_t");
      if ( is_memo || is_text )
      {
        if ( match == nullptr )
          match = w;
        return; // don't descend into the matched surface
      }
      const auto kids = w->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
      for ( QWidget *k : kids )
        walk(k);
    };
    walk(root);
    return match;
  };

  // 1) Preferred: the actual paint surface inside the active viewer. The
  //    active viewer is the top-level IDA "view" widget, but the actual
  //    QWidget we want to cover is its CustomIDAMemo / text_area_t child
  //    (the one whose paint is the disassembly / pseudocode).
  TWidget *viewer = get_current_viewer();
  if ( viewer != nullptr )
  {
    if ( QWidget *surface = find_paint_surface((QWidget *)viewer) )
      return surface;
    return (QWidget *) viewer;
  }

  // 2) Active widget (any focus widget) - try the same surface search first.
  TWidget *cur = get_current_widget();
  if ( cur != nullptr )
  {
    if ( QWidget *surface = find_paint_surface((QWidget *)cur) )
      return surface;
    return (QWidget *) cur;
  }

  return nullptr;
}

void DokiOverlayManager::attach_to_current()
{
  if ( !m_enabled )
  {
    if ( m_overlay )
    {
      m_overlay->clear_sticker();
      m_overlay->clear_target();
    }
    return;
  }

  QWidget *target = choose_target_widget();
  if ( target == nullptr )
  {
    if ( m_overlay )   m_overlay->hide();
    doki::msg_log("  overlay: no active viewer/widget to attach to\n");
    return;
  }

  if ( m_overlay && m_enabled )
    m_overlay->set_target(target);

  if ( m_overlay && m_enabled )
    m_overlay->raise();
}

ssize_t idaapi DokiOverlayManager::on_event(ssize_t code, va_list va)
{
  // Ignore UI churn while the user has disabled the sticker. This prevents
  // minimize/restore, active-widget changes, and child-widget swaps from
  // reattaching or showing a stale overlay.
  if ( !m_enabled )
    return 0;

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
