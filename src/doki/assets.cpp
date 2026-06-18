//----------------------------------------------------------------------------
// Doki Theme - on-demand asset manager implementation.
//
// Uses Qt Network (QNetworkAccessManager + QEventLoop) to fetch from the
// authoritative Doki CDN with a strict, short timeout. Files are validated
// as decodable PNGs before being atomically promoted into the cache.
//
// Everything is best-effort: on any failure path we log a concise warning,
// remove any temp file, and return an empty path. Colors and the nav
// colorizer still apply when an asset is unavailable.
//----------------------------------------------------------------------------
#include "doki/assets.h"
#include "doki/log.h"
#include "doki/paths.h"

#include <ida.hpp>
#include <kernwin.hpp>

#include <QEventLoop>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QString>
#include <QTimer>
#include <QtCore>

#include <atomic>
#include <mutex>
#include <set>
#include <string>

namespace doki
{

// ---------------------------------------------------------------------------
// CDN configuration. The host/scheme are intentionally hard-coded here
// and never derived from the theme JSON.
// ---------------------------------------------------------------------------
namespace
{

constexpr int kRequestTimeoutMs = 15000;   // bounded, generous
constexpr int kConnectTimeoutMs = 8000;

const char *kCdnHost = "https://doki.assets.unthrottled.io";

const char *kind_prefix(AssetKind k)
{
  return k == AssetKind::Sticker ? "stickers" : "backgrounds";
}

const char *kind_cache_subdir(AssetKind k)
{
  return k == AssetKind::Sticker ? "stickers" : "wallpapers";
}

// Re-entrancy guard: at most one in-flight request per (kind, file_name)
// within a process. Stays in module scope so the static in-flight set
// is shared across all callers.
std::mutex &inflight_mutex()
{
  static std::mutex m;
  return m;
}

std::set<std::string> &inflight_set()
{
  static std::set<std::string> s;
  return s;
}

std::string inflight_key(AssetKind k, const std::string &name)
{
  return std::string(kind_prefix(k)) + ":" + name;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Validation / URL construction (header docstrings live in assets.h)
// ---------------------------------------------------------------------------

bool is_valid_remote_path(AssetKind kind, const std::string &remote_path)
{
  if ( remote_path.empty() )
    return false;
  // Disallow backslashes; the CDN serves forward-slash paths.
  if ( remote_path.find('\\') != std::string::npos )
    return false;
  // Reject traversal segments and protocol markers.
  if ( remote_path.find("..") != std::string::npos )
    return false;
  if ( remote_path.find("://") != std::string::npos )
    return false;
  // Must start with the expected category prefix and have at least one more
  // slash + non-empty file name.
  const std::string prefix = std::string(kind_prefix(kind)) + "/";
  if ( remote_path.compare(0, prefix.size(), prefix) != 0 )
    return false;
  if ( remote_path.size() <= prefix.size() )
    return false;
  const std::string tail = remote_path.substr(prefix.size());
  if ( tail.empty() || tail.back() == '/' )
    return false;
  // Reject any empty path segment (which would indicate "a//b").
  for ( size_t i = 0; i < tail.size(); ++i )
  {
    if ( tail[i] == '/' && (i + 1 < tail.size()) && tail[i + 1] == '/' )
      return false;
  }
  return true;
}

std::string cdn_url_for(AssetKind kind, const std::string &remote_path)
{
  if ( !is_valid_remote_path(kind, remote_path) )
    return std::string();
  return std::string(kCdnHost) + "/" + remote_path;
}

std::string cache_dir(AssetKind kind)
{
  return path_join(path_join(doki_root(), "cache"),
                   kind_cache_subdir(kind));
}

std::string cache_file_path(AssetKind kind, const std::string &file_name)
{
  // Defensive: the file name is treated as a leaf only. Reject anything
  // that would escape the cache directory.
  if ( file_name.empty()
    || file_name.find('/')  != std::string::npos
    || file_name.find('\\') != std::string::npos
    || file_name.find("..") != std::string::npos )
  {
    return std::string();
  }
  return path_join(cache_dir(kind), file_name);
}

// ---------------------------------------------------------------------------
// Atomic file write: write to <path>.tmp.<rand>, validate as PNG, then
// rename to <path>. If any step fails, remove the temp file.
// ---------------------------------------------------------------------------
namespace
{

bool is_decodable_png(const QString &path)
{
  QImageReader r(path);
  r.setFormat("png");
  if ( !r.canRead() )
    return false;
  QImage img = r.read();
  return !img.isNull();
}

QString unique_temp_path(const std::string &final_path)
{
  // Use a process-local counter to make unique temp names without
  // depending on std::random_device for reproducibility.
  static std::atomic<unsigned long> s_seq{ 0 };
  const unsigned long seq = s_seq.fetch_add(1, std::memory_order_relaxed);
  // pro.h #defines snprintf to dont_use_snprintf; build the suffix
  // with std::to_string + QByteArray::number instead.
  const QByteArray fname = QFileInfo(QString::fromStdString(final_path))
                               .fileName().toUtf8();
  QString tmp = QString::fromStdString(final_path)
              + QString::fromUtf8(".")
              + QString::fromUtf8(fname)
              + QString::fromUtf8(".tmp.")
              + QString::number(static_cast<qulonglong>(seq));
  return tmp;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ensure_asset
// ---------------------------------------------------------------------------
std::string ensure_asset(AssetKind kind,
                         const std::string &file_name,
                         const std::string &remote_path)
{
  // 1) Defensive validation of inputs (the caller is supposed to have
  //    done this, but trust nothing).
  if ( !is_valid_remote_path(kind, remote_path) )
  {
    doki::msg_log("  assets: invalid remote path for %s '%s'\n",
                  kind_prefix(kind), file_name.c_str());
    return std::string();
  }
  const std::string final_path = cache_file_path(kind, file_name);
  if ( final_path.empty() )
  {
    doki::msg_log("  assets: invalid cache file name '%s'\n",
                  file_name.c_str());
    return std::string();
  }

  // 2) Cache hit?
  if ( QFileInfo(QString::fromStdString(final_path)).isFile() )
    return final_path;

  // 3) Coalesce concurrent fetches for the same key.
  const std::string key = inflight_key(kind, file_name);
  {
    std::lock_guard<std::mutex> lk(inflight_mutex());
    if ( inflight_set().count(key) != 0 )
    {
      // Another call is already fetching this. Bail; the caller will
      // retry on a later apply pass and pick up the cache when ready.
      doki::msg_log("  assets: fetch already in flight for %s '%s'\n",
                    kind_prefix(kind), file_name.c_str());
      return std::string();
    }
    inflight_set().insert(key);
  }

  // RAII guard to clear the in-flight flag on every exit path.
  struct InflightGuard
  {
    std::string key;
    ~InflightGuard() {
      std::lock_guard<std::mutex> lk(inflight_mutex());
      inflight_set().erase(key);
    }
  } guard{ key };

  // 4) Ensure cache directory exists, then download to a temp file.
  if ( !ensure_dir(cache_dir(kind)) )
  {
    doki::msg_log("  assets: cannot create cache dir %s\n",
                  cache_dir(kind).c_str());
    return std::string();
  }

  // In headless / batch / tty_idalib runs there is no Qt event loop that
  // the network stack can drive, so attempting a download would either
  // deadlock or spin the nested loop forever. Skip the fetch in those
  // contexts: the legacy offline fallback (or a later GUI apply) is the
  // path the user gets artwork through.
  if ( !is_idaq() || QApplication::instance() == nullptr )
  {
    doki::msg_log("  assets: skipping fetch in non-GUI run for %s '%s'\n",
                  kind_prefix(kind), file_name.c_str());
    return std::string();
  }

  const QString url = QString::fromStdString(
      cdn_url_for(kind, remote_path));
  const QString tmp_path = unique_temp_path(final_path);

  // 5) Fetch. Use a short-lived QNetworkAccessManager and a nested event
  //    loop with a strict timeout. This is the only safe way to do HTTP
  //    from an IDA action handler on the GUI thread.
  QNetworkAccessManager nam;
  QNetworkRequest req(url);
  req.setRawHeader("User-Agent", "doki-theme-ida/0.1");
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);

  QNetworkReply *reply = nam.get(req);
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, reply, [reply]() {
    if ( reply->isRunning() )
      reply->abort();
  });
  QObject::connect(reply, &QNetworkReply::errorOccurred, reply,
      [&timeout](QNetworkReply::NetworkError) { /* handled in finished() */ });
  timeout.start(kRequestTimeoutMs);
  loop.exec();

  if ( reply->error() != QNetworkReply::NoError )
  {
    doki::msg_log("  assets: fetch failed for %s: %s\n",
                  file_name.c_str(),
                  reply->errorString().toUtf8().constData());
    reply->deleteLater();
    QFile::remove(tmp_path);
    return std::string();
  }

  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if ( status < 200 || status >= 300 )
  {
    doki::msg_log("  assets: HTTP %d for %s\n", status, file_name.c_str());
    reply->deleteLater();
    QFile::remove(tmp_path);
    return std::string();
  }

  // 6) Write the body to a temp file.
  {
    QFile tmp(tmp_path);
    if ( !tmp.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
      doki::msg_log("  assets: cannot open temp file %s\n",
                    tmp_path.toUtf8().constData());
      reply->deleteLater();
      return std::string();
    }
    const QByteArray body = reply->readAll();
    if ( body.isEmpty() )
    {
      doki::msg_log("  assets: empty body for %s\n", file_name.c_str());
      reply->deleteLater();
      tmp.close();
      QFile::remove(tmp_path);
      return std::string();
    }
    if ( tmp.write(body) != body.size() )
    {
      doki::msg_log("  assets: short write to %s\n",
                    tmp_path.toUtf8().constData());
      reply->deleteLater();
      tmp.close();
      QFile::remove(tmp_path);
      return std::string();
    }
    tmp.close();
  }
  reply->deleteLater();

  // 7) Validate the temp file is a decodable PNG.
  if ( !is_decodable_png(tmp_path) )
  {
    doki::msg_log("  assets: downloaded file is not a valid PNG: %s\n",
                  file_name.c_str());
    QFile::remove(tmp_path);
    return std::string();
  }

  // 8) Atomically promote to the final cache path.
  //    QFile::rename fails if the destination exists; remove first, then
  //    rename. On Windows this is not strictly atomic but Qt's rename
  //    uses MoveFileEx with replace semantics, which is atomic enough
  //    for our purposes.
  if ( QFile::exists(QString::fromStdString(final_path)) )
    QFile::remove(QString::fromStdString(final_path));
  if ( !QFile::rename(tmp_path, QString::fromStdString(final_path)) )
  {
    doki::msg_log("  assets: cannot promote temp to cache for %s\n",
                  file_name.c_str());
    QFile::remove(tmp_path);
    return std::string();
  }

  doki::msg_log("  assets: cached %s/%s -> %s\n",
                kind_prefix(kind), file_name.c_str(),
                final_path.c_str());
  return final_path;
}

} // namespace doki
