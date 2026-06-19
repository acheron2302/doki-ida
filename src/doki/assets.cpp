//----------------------------------------------------------------------------
// Doki Theme - on-demand asset manager implementation.
//
// Uses libcurl (linked statically into the plugin via CMake FetchContent,
// with Schannel on Windows and OpenSSL on Linux/macOS) to fetch from the
// authoritative Doki CDN with a strict, short timeout. This intentionally
// avoids Qt Network so HTTPS still works with IDA/Qt builds configured
// with QT_NO_SSL. Files are validated as decodable PNGs before being
// atomically promoted into the cache.
//
// If the static libcurl build is unavailable at configure time we fall
// back to loading libcurl dynamically via QLibrary so the plugin still
// works in environments where FetchContent was skipped (e.g. -DOKI_USE_EMBEDDED_CURL=OFF).
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

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLibrary>
#include <QString>
#include <QStringList>
#include <QtCore>

#include <atomic>
#include <mutex>
#include <set>
#include <string>

#if DOKI_HAS_EMBEDDED_CURL
#include <curl/curl.h>
#endif

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

// One-shot downloader diagnostics. We intentionally do not use Qt Network:
// IDA can ship a Qt build with QT_NO_SSL, which makes QNetworkAccessManager
// unable to handle https:// URLs. libcurl is cross-platform and certificate
// verification remains enabled.
std::once_flag g_downloader_diag_once;

constexpr size_t kMaxAssetBytes = 30u * 1024u * 1024u;

// ---------------------------------------------------------------------------
// Optional dynamic libcurl loader. Only used when the static libcurl build
// is not available (DOKI_HAS_EMBEDDED_CURL is 0). At static-link time we
// just call the libcurl API directly.
// ---------------------------------------------------------------------------
#if !DOKI_HAS_EMBEDDED_CURL

constexpr int CURLE_OK = 0;
constexpr int CURLE_COULDNT_RESOLVE_HOST = 6;
constexpr int CURLE_COULDNT_CONNECT = 7;
constexpr int CURLE_OPERATION_TIMEDOUT = 28;
constexpr int CURLE_SSL_CONNECT_ERROR = 35;
constexpr int CURLE_PEER_FAILED_VERIFICATION = 60;
constexpr int CURLE_SSL_CACERT_BADFILE = 77;

constexpr long CURL_GLOBAL_DEFAULT = 3;
constexpr long CURLPROTO_HTTPS = 1L << 1;

constexpr int CURLOPT_WRITEDATA = 10001;
constexpr int CURLOPT_URL = 10002;
constexpr int CURLOPT_USERAGENT = 10018;
constexpr int CURLOPT_WRITEFUNCTION = 20011;
constexpr int CURLOPT_FOLLOWLOCATION = 52;
constexpr int CURLOPT_SSL_VERIFYPEER = 64;
constexpr int CURLOPT_SSL_VERIFYHOST = 81;
constexpr int CURLOPT_MAXREDIRS = 68;
constexpr int CURLOPT_CONNECTTIMEOUT_MS = 156;
constexpr int CURLOPT_TIMEOUT_MS = 155;
constexpr int CURLOPT_PROTOCOLS = 181;
constexpr int CURLOPT_REDIR_PROTOCOLS = 182;

constexpr int CURLINFO_RESPONSE_CODE = 0x200000 + 2;

#if defined(_WIN32)
#define DOKI_CURL_CALL __cdecl
#else
#define DOKI_CURL_CALL
#endif

using CURL = void;
using CURLcode = int;

using curl_global_init_t = CURLcode (DOKI_CURL_CALL *)(long flags);
using curl_global_cleanup_t = void (DOKI_CURL_CALL *)();
using curl_easy_init_t = CURL *(DOKI_CURL_CALL *)();
using curl_easy_cleanup_t = void (DOKI_CURL_CALL *)(CURL *handle);
using curl_easy_perform_t = CURLcode (DOKI_CURL_CALL *)(CURL *handle);
using curl_easy_setopt_t = CURLcode (DOKI_CURL_CALL *)(CURL *handle, int, ...);
using curl_easy_getinfo_t = CURLcode (DOKI_CURL_CALL *)(CURL *handle, int, ...);
using curl_easy_strerror_t = const char *(DOKI_CURL_CALL *)(CURLcode code);

struct CurlApi
{
  QLibrary lib;
  curl_global_init_t global_init = nullptr;
  curl_global_cleanup_t global_cleanup = nullptr;
  curl_easy_init_t easy_init = nullptr;
  curl_easy_cleanup_t easy_cleanup = nullptr;
  curl_easy_perform_t easy_perform = nullptr;
  curl_easy_setopt_t easy_setopt = nullptr;
  curl_easy_getinfo_t easy_getinfo = nullptr;
  curl_easy_strerror_t easy_strerror = nullptr;
  bool ok = false;

  CurlApi()
  {
    const char *names[] = {
#if defined(_WIN32)
      "libcurl", "libcurl-x64", "libcurl-4", "curl"
#elif defined(__APPLE__)
      "libcurl.4", "libcurl", "curl"
#else
      "libcurl.so.4", "libcurl", "curl"
#endif
    };

    QStringList dirs;
    dirs << QString(); // default platform loader paths
    const QString appDir = QCoreApplication::applicationDirPath();
    if ( !appDir.isEmpty() )
    {
      dirs << appDir;
      dirs << appDir + QStringLiteral("/plugins");
    }
    const std::string userPlugins = path_join(user_idadir(), "plugins");
    if ( !userPlugins.empty() )
      dirs << QString::fromStdString(userPlugins);
    dirs << QString::fromStdString(path_join(doki_root(), "bin"));

    for ( const QString &dir : dirs )
    {
      for ( const char *name : names )
      {
        const QString qname = QString::fromLatin1(name);
        lib.setFileName(dir.isEmpty()
                        ? qname
                        : dir + QStringLiteral("/") + qname);
        if ( lib.load() )
          break;
      }
      if ( lib.isLoaded() )
        break;
    }
    if ( !lib.isLoaded() )
      return;

    global_init = reinterpret_cast<curl_global_init_t>(
        lib.resolve("curl_global_init"));
    global_cleanup = reinterpret_cast<curl_global_cleanup_t>(
        lib.resolve("curl_global_cleanup"));
    easy_init = reinterpret_cast<curl_easy_init_t>(
        lib.resolve("curl_easy_init"));
    easy_cleanup = reinterpret_cast<curl_easy_cleanup_t>(
        lib.resolve("curl_easy_cleanup"));
    easy_perform = reinterpret_cast<curl_easy_perform_t>(
        lib.resolve("curl_easy_perform"));
    easy_setopt = reinterpret_cast<curl_easy_setopt_t>(
        lib.resolve("curl_easy_setopt"));
    easy_getinfo = reinterpret_cast<curl_easy_getinfo_t>(
        lib.resolve("curl_easy_getinfo"));
    easy_strerror = reinterpret_cast<curl_easy_strerror_t>(
        lib.resolve("curl_easy_strerror"));

    if ( global_init == nullptr || global_cleanup == nullptr
      || easy_init == nullptr || easy_cleanup == nullptr
      || easy_perform == nullptr || easy_setopt == nullptr
      || easy_getinfo == nullptr || easy_strerror == nullptr )
    {
      lib.unload();
      return;
    }

    ok = (global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  }

  ~CurlApi()
  {
    if ( ok && global_cleanup != nullptr )
      global_cleanup();
  }
};

CurlApi &curl_api()
{
  static CurlApi api;
  return api;
}

#endif // !DOKI_HAS_EMBEDDED_CURL

const char *curl_hint(int code)
{
  switch ( code )
  {
    case CURLE_PEER_FAILED_VERIFICATION:
    case CURLE_SSL_CACERT_BADFILE:
    case CURLE_SSL_CONNECT_ERROR:
      return "TLS/certificate validation failed. Certificate verification is "
             "enabled; make sure libcurl can find a CA bundle/trust store and "
             "check the system clock or TLS-inspection proxy.";
    case CURLE_OPERATION_TIMEDOUT:
      return "network timeout. Check connectivity to the doki-theme CDN or "
             "pre-warm the cache with tools\\fetch_assets.ps1.";
    case CURLE_COULDNT_RESOLVE_HOST:
      return "DNS resolution failed for the CDN host.";
    case CURLE_COULDNT_CONNECT:
      return "could not connect to the CDN host; check proxy/firewall rules.";
    default:
      return "libcurl is used instead of Qt Network, so Qt SSL/OpenSSL "
             "support is not required; deploy libcurl and its TLS backend "
             "dependencies if downloads fail.";
  }
}

size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  QByteArray *body = static_cast<QByteArray *>(userdata);
  const size_t bytes = size * nmemb;
  if ( size != 0 && bytes / size != nmemb )
    return 0;
  if ( static_cast<size_t>(body->size()) + bytes > kMaxAssetBytes )
    return 0;
  body->append(ptr, static_cast<int>(bytes));
  return bytes;
}

#if DOKI_HAS_EMBEDDED_CURL

bool download_asset_body(const std::string &url,
                         AssetKind kind,
                         const std::string &file_name,
                         QByteArray *body,
                         long *http_status)
{
  body->clear();
  *http_status = 0;

  static std::once_flag init_once;
  static bool init_ok = false;
  std::call_once(init_once, []()
  {
    init_ok = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  });
  if ( !init_ok )
  {
    doki::msg_log("  assets: curl_global_init failed; cannot fetch %s file "
                  "'%s' from '%s'\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str());
    return false;
  }

  CURL *curl = curl_easy_init();
  if ( curl == nullptr )
  {
    doki::msg_log("  assets: curl_easy_init failed for %s file '%s'\n",
                  kind_prefix(kind), file_name.c_str());
    return false;
  }
  struct EasyGuard
  {
    CURL *curl;
    ~EasyGuard() { curl_easy_cleanup(curl); }
  } guard{ curl };

  CURLcode rc = CURLE_OK;
  auto setopt = [&](CURLoption opt, auto value) -> bool
  {
    rc = curl_easy_setopt(curl, opt, value);
    return rc == CURLE_OK;
  };

  if ( !setopt(CURLOPT_URL, url.c_str())
    || !setopt(CURLOPT_USERAGENT, "doki-theme-ida/0.1")
    || !setopt(CURLOPT_WRITEFUNCTION, curl_write_cb)
    || !setopt(CURLOPT_WRITEDATA, body)
    || !setopt(CURLOPT_FOLLOWLOCATION, 1L)
    || !setopt(CURLOPT_MAXREDIRS, 5L)
    || !setopt(CURLOPT_CONNECTTIMEOUT_MS,
               static_cast<long>(kConnectTimeoutMs))
    || !setopt(CURLOPT_TIMEOUT_MS,
               static_cast<long>(kRequestTimeoutMs))
    || !setopt(CURLOPT_SSL_VERIFYPEER, 1L)
    || !setopt(CURLOPT_SSL_VERIFYHOST, 2L)
    || !setopt(CURLOPT_PROTOCOLS, CURLPROTO_HTTPS)
    || !setopt(CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS) )
  {
    doki::msg_log("  assets: curl_easy_setopt failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), curl_easy_strerror(rc));
    return false;
  }

  rc = curl_easy_perform(curl);
  if ( rc != CURLE_OK )
  {
    doki::msg_log("  assets: libcurl fetch failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), curl_easy_strerror(rc));
    doki::msg_log("  assets: hint: %s\n", curl_hint(rc));
    return false;
  }

  rc = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_status);
  if ( rc != CURLE_OK )
  {
    doki::msg_log("  assets: curl_easy_getinfo failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), curl_easy_strerror(rc));
    return false;
  }

  return true;
}

#else // dynamic loader fallback

bool download_asset_body(const std::string &url,
                         AssetKind kind,
                         const std::string &file_name,
                         QByteArray *body,
                         long *http_status)
{
  body->clear();
  *http_status = 0;

  CurlApi &api = curl_api();
  if ( !api.ok )
  {
    doki::msg_log("  assets: libcurl not available; cannot fetch %s file "
                  "'%s' from '%s'\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str());
    doki::msg_log("  assets: hint: deploy libcurl (with HTTPS/TLS support) "
                  "next to the plugin/IDA executable, or pre-warm the cache "
                  "with tools\\fetch_assets.ps1. Colors/nav styling still "
                  "apply without artwork.\n");
    return false;
  }

  CURL *curl = api.easy_init();
  if ( curl == nullptr )
  {
    doki::msg_log("  assets: curl_easy_init failed for %s file '%s'\n",
                  kind_prefix(kind), file_name.c_str());
    return false;
  }

  struct EasyGuard
  {
    CurlApi &api;
    CURL *curl;
    ~EasyGuard() { api.easy_cleanup(curl); }
  } guard{ api, curl };

  CURLcode rc = CURLE_OK;
  auto setopt = [&](int opt, auto value) -> bool
  {
    rc = api.easy_setopt(curl, opt, value);
    return rc == CURLE_OK;
  };

  if ( !setopt(CURLOPT_URL, url.c_str())
    || !setopt(CURLOPT_USERAGENT, "doki-theme-ida/0.1")
    || !setopt(CURLOPT_WRITEFUNCTION, curl_write_cb)
    || !setopt(CURLOPT_WRITEDATA, body)
    || !setopt(CURLOPT_FOLLOWLOCATION, 1L)
    || !setopt(CURLOPT_MAXREDIRS, 5L)
    || !setopt(CURLOPT_CONNECTTIMEOUT_MS,
               static_cast<long>(kConnectTimeoutMs))
    || !setopt(CURLOPT_TIMEOUT_MS,
               static_cast<long>(kRequestTimeoutMs))
    || !setopt(CURLOPT_SSL_VERIFYPEER, 1L)
    || !setopt(CURLOPT_SSL_VERIFYHOST, 2L)
    || !setopt(CURLOPT_PROTOCOLS, CURLPROTO_HTTPS)
    || !setopt(CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS) )
  {
    const char *err = api.easy_strerror(rc);
    doki::msg_log("  assets: curl_easy_setopt failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), err ? err : "unknown");
    return false;
  }

  rc = api.easy_perform(curl);
  if ( rc != CURLE_OK )
  {
    const char *err = api.easy_strerror(rc);
    doki::msg_log("  assets: libcurl fetch failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), err ? err : "unknown");
    doki::msg_log("  assets: hint: %s\n", curl_hint(rc));
    return false;
  }

  rc = api.easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_status);
  if ( rc != CURLE_OK )
  {
    const char *err = api.easy_strerror(rc);
    doki::msg_log("  assets: curl_easy_getinfo failed for %s file '%s' "
                  "url '%s' (code=%d %s)\n",
                  kind_prefix(kind), file_name.c_str(), url.c_str(),
                  static_cast<int>(rc), err ? err : "unknown");
    return false;
  }

  return true;
}

#endif // DOKI_HAS_EMBEDDED_CURL

void log_downloader_diagnostics()
{
  std::call_once(g_downloader_diag_once, []()
  {
    doki::msg_log("  assets: HTTPS downloader diagnostics\n");
#if DOKI_HAS_EMBEDDED_CURL
    doki::msg_log("    downloader          : libcurl (statically linked "
                  "into the plugin, no Qt SSL dependency)\n");
    doki::msg_log("    libcurl loaded      : yes\n");
#else
    CurlApi &api = curl_api();
    doki::msg_log("    downloader          : libcurl (dynamic load, "
                  "no Qt SSL dependency)\n");
    doki::msg_log("    libcurl loaded      : %s\n", api.ok ? "yes" : "NO");
    if ( api.ok )
      doki::msg_log("    libcurl library     : %s\n",
                    api.lib.fileName().toUtf8().constData());
#endif
    doki::msg_log("    TLS verification    : enabled "
                  "(CURLOPT_SSL_VERIFYPEER=1, VERIFYHOST=2)\n");
    doki::msg_log("    protocols           : HTTPS only; HTTPS->HTTP "
                  "redirects disabled\n");
    doki::msg_log("    CDN host            : %s\n", kCdnHost);
    doki::msg_log("    timeouts            : connect=%dms total=%dms\n",
                  kConnectTimeoutMs, kRequestTimeoutMs);
  });
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

  // Surface downloader capability exactly once per process. We do this
  // before any network request so a missing libcurl deployment is obvious
  // without depending on a specific reply to trigger it.
  log_downloader_diagnostics();
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

  const std::string url = cdn_url_for(kind, remote_path);
  const QString tmp_path = unique_temp_path(final_path);

  // 5) Fetch. libcurl is statically linked into the plugin (preferred) or
  //    loaded dynamically (fallback). In both cases Qt Network/SSL is not
  //    required. If libcurl is missing entirely, this returns false and
  //    the caller still gets colors/nav styling without artwork.
  QByteArray body;
  long status = 0;
  if ( !download_asset_body(url, kind, file_name, &body, &status) )
  {
    QFile::remove(tmp_path);
    return std::string();
  }

  if ( status < 200 || status >= 300 )
  {
    doki::msg_log("  assets: HTTP %ld for %s file '%s' url '%s'\n",
                  status, kind_prefix(kind), file_name.c_str(),
                  url.c_str());
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
      return std::string();
    }
    if ( body.isEmpty() )
    {
      doki::msg_log("  assets: empty body for %s\n", file_name.c_str());
      tmp.close();
      QFile::remove(tmp_path);
      return std::string();
    }
    if ( tmp.write(body) != body.size() )
    {
      doki::msg_log("  assets: short write to %s\n",
                    tmp_path.toUtf8().constData());
      tmp.close();
      QFile::remove(tmp_path);
      return std::string();
    }
    tmp.close();
  }

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
