//----------------------------------------------------------------------------
// Doki Theme - configuration persistence implementation.
//----------------------------------------------------------------------------
// nlohmann/json must precede any IDA SDK header: pro.h #defines snprintf to
// 'dont_use_snprintf', which would break json.hpp's internal std::snprintf use.
#include <nlohmann/json.hpp>
#include <fstream>

#include "doki/config.h"
#include "doki/paths.h"
#include "doki/log.h"

using json = nlohmann::json;

namespace doki
{

static std::string config_path()
{
  return path_join(doki_root(), "config.json");
}

DokiConfig load_config()
{
  DokiConfig cfg;
  std::ifstream ifs(config_path(), std::ios::binary);
  if ( !ifs )
    return cfg;

  try
  {
    json j;
    ifs >> j;
    cfg.selected_id                = j.value("selectedId", std::string());
    cfg.sticker_enabled            = j.value("stickerEnabled", true);
    cfg.wallpaper_enabled          = j.value("wallpaperEnabled", true);
    cfg.original_theme             = j.value("originalTheme", std::string());
    cfg.live_nav_colorizer_enabled = j.value("liveNavColorizerEnabled", false);

    // Legacy fields from the removed coordinated-background / glass-pane
    // feature are silently ignored. They will disappear on the next save
    // because save_config() no longer writes them.
  }
  catch ( const std::exception &e )
  {
    doki::msg_log("config parse error (using defaults): %s\n", e.what());
    return DokiConfig();
  }
  return cfg;
}

bool save_config(const DokiConfig &cfg)
{
  if ( !ensure_dir(doki_root()) )
    return false;

  json j;
  j["selectedId"]              = cfg.selected_id;
  j["stickerEnabled"]          = cfg.sticker_enabled;
  j["wallpaperEnabled"]        = cfg.wallpaper_enabled;
  j["originalTheme"]           = cfg.original_theme;
  j["liveNavColorizerEnabled"] = cfg.live_nav_colorizer_enabled;

  std::ofstream ofs(config_path(), std::ios::binary | std::ios::trunc);
  if ( !ofs )
    return false;
  const std::string s = j.dump(2);
  ofs.write(s.data(), (std::streamsize)s.size());
  return (bool)ofs;
}

} // namespace doki
